#!/usr/bin/env python3
"""只读运行监控与有限时长实板压力验证；不复位、不操作SD或写入型自检。"""
import argparse
import json
import os
from pathlib import Path
import socket
import signal
import struct
import subprocess
import tempfile
import time
from scope_link_probe import ScopeProbe
from lower_probe import attach
from response_probe import ROOT, require, METRICS

FIELDS = ('uptime_ms epoch cycles_per_ms heap_free heap_min heap_total lv_used lv_peak lv_total '
          'ram_used ccm_used external_used rtt_dropped rx_frames rx_errors mouse_dropped reset_flags '
          'fault_kind fault_pc fault_lr fault_cfsr fault_hfsr').split()


def monitor(p):
    data,_=p.request(16)
    require(len(data)>=428 and data[0]==1 and data[5:8]==bytes((0,18,7)), 'monitor schema')
    count,total,status=data[1],struct.unpack_from('<H',data,2)[0],data[4]
    require(len(data)==428+25*count and count<=10 and status==0 and count==total==6,'task snapshot')
    m=dict(zip(FIELDS,struct.unpack_from('<22I',data,8)))
    m['faultTask']=data[96:112].split(b'\0')[0].decode()
    m['metrics']={name:dict(zip(('count','last','peak','over20ms'),struct.unpack_from('<4I',data,112+16*i))) for i,name in enumerate(METRICS)}
    m['flow']=list(struct.unpack_from('<7I',data,400))
    m['tasks']=[]
    for i in range(count):
        offset=428+25*i
        name=data[offset:offset+16].split(b'\0')[0].decode()
        priority,free,state=struct.unpack_from('<IIc',data,offset+16)
        m['tasks'].append(dict(name=name,priority=priority,stackFree=free,state=state.decode()))
    require([x['name'] for x in m['tasks']]==sorted(x['name'] for x in m['tasks']),'task order')
    require(0<m['heap_min']<=m['heap_free']<=m['heap_total'],'heap bounds')
    require(0<m['lv_used']<=m['lv_peak']<=m['lv_total'],'LVGL bounds')
    require(not(m['reset_flags'] & (1<<29)) and m['fault_kind']==0,'reset/fault')
    require(m['cycles_per_ms']==168000,'timebase')
    return m


def suite(p, seconds, backpressure):
    p.add_symbols()
    p.status(16,b'x',1)
    first=monitor(p)
    print('INITIAL '+json.dumps(first),flush=True)
    # 两个短命令进入同一批下行，StationTask尚未消费时第二条必须有界返回BUSY。
    keys=[];frames=[]
    for _ in range(2):
        p.seq=(p.seq+1)%256;key=(0x90,p.seq)
        require(key not in p.pending and key not in p.done,'sequence collision')
        import ctypes
        frame=ctypes.create_string_buffer(1034)
        n=p.library.vc_encode(frame,1034,16,p.seq,b'',0)
        frames.append(frame.raw[:n]);p.pending[key]=time.monotonic();keys.append(key)
    p.rtt.sendall(b''.join(frames))
    codes=sorted(p.collect(key)[0][0] for key in keys)
    require(codes==[0,3],f'monitor slot: {codes}')
    print('PASS invalid length and monitor BUSY',flush=True)
    baseline=first
    if backpressure:
        p.request(5,struct.pack('<H',1))
        # 暂停OpenOCD进程，MCU仍运行；仅rtt stop在本机仍会排空上行区，不能作为满缓冲证据。
        require(p.proc.poll() is None,'OpenOCD not running')
        p.proc.send_signal(signal.SIGSTOP)
        try:
            time.sleep(6)
        finally:
            p.proc.send_signal(signal.SIGCONT)
        p.pump(.5);p.request(5,struct.pack('<H',50))
        baseline=monitor(p)
        print('PRESSURE CHECK '+json.dumps({key:baseline[key] for key in ('uptime_ms','rtt_dropped')}),flush=True)
        require(baseline['rtt_dropped']>first['rtt_dropped'],'backpressure did not fill device ring')
        require(baseline['uptime_ms']>first['uptime_ms'],'reset during backpressure')
        p.request(1,b'recovered-after-full-ring')
        print('BACKPRESSURE '+json.dumps(baseline),flush=True)
        print('PASS actual full-ring timeout/drop, intact frame synchronization and command recovery',flush=True)
    start=time.monotonic();next_page=start+30;next_report=start;frames=0;pings=[];samples=[];reconnects=0
    while time.monotonic()-start<seconds:
        before=time.monotonic()
        meta=p.begin(50000,1024)
        for _ in range(2):pings.append(p.request(1,b'x'*512)[1])
        m=monitor(p)
        p.samples(meta);frames+=1
        for _ in range(2):pings.append(p.request(1,b'x'*512)[1])
        require(m['uptime_ms']>=baseline['uptime_ms'],'unexpected reset/uptime regression')
        require(m['rtt_dropped']==baseline['rtt_dropped'],'unexpected RTT drop outside induced backpressure')
        require(m['rx_errors']==baseline['rx_errors'],'device decoder errors')
        require(m['mouse_dropped']==baseline['mouse_dropped'],'mouse queue drops')
        require(m['heap_min']==baseline['heap_min'],'FreeRTOS heap high-water regression')
        require(all(t['stackFree']>=256 for t in m['tasks']),'low stack margin')
        if time.monotonic()>=next_page:
            p.tap(40,38);require(p.title()=='VISIONCRAFT','back failed')
            p.tap(392,190);require(p.title()=='DEVICE','DEVICE failed')
            next_page+=30
        # 重新打开RTT客户端，不复位固件；无读者窗口短，不应填满环形区。
        if frames%60==0:
            p.rtt.close();p.rtt=socket.create_connection(('127.0.0.1',p.rtt_port),3)
            p.rtt.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1);p.rtt.settimeout(.02)
            p.request(1,b'reconnected');reconnects+=1
        if time.monotonic()>=next_report:
            entry=dict(elapsed=round(time.monotonic()-start,2),monitor=m)
            samples.append(entry);print('SAMPLE '+json.dumps(entry),flush=True)
            next_report+=10
        p.pump(max(0,.5-(time.monotonic()-before)))
    p.request(15);p.stopped();last=monitor(p)
    require(last['lv_used']<=baseline['lv_used']+1024,'LVGL memory grows on repeated recreation')
    result=dict(seconds=round(time.monotonic()-start,2),frames=frames,pings=len(pings),max_ping_ms=round(max(pings,default=0),3),
                reconnects=reconnects,initial=first,after_backpressure=baseline,final=last,samples=samples)
    (p.folder/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS requested runtime window; covered workload is reported by counts below; no storage operations',flush=True)
    print(json.dumps({k:v for k,v in result.items() if k not in ('initial','after_backpressure','final','samples')}),flush=True)


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--seconds',type=int,default=600)
    ap.add_argument('--backpressure',action='store_true')
    args=ap.parse_args();require(0<=args.seconds<=86400,'seconds out of range');args.output.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='vc-monitor-') as tmp:
        lib=Path(tmp)/'protocol.so';flags=['--sysroot='+os.environ['VC_SYSROOT']] if os.environ.get('VC_SYSROOT') else []
        subprocess.run(['cc',*flags,'-shared','-fPIC',str(ROOT/'protocol/vc_protocol.c'),'-o',str(lib)],check=True)
        p=ScopeProbe(args.output,6679,19079,lib);ready=False
        try:
            attach(p);ready=True;suite(p,args.seconds,args.backpressure)
        finally:
            try:
                if ready:p.request(15)
            finally:p.close()


if __name__=='__main__':main()
