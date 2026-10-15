#!/usr/bin/env python3
"""A03 实板非存储实验。先编译/烧录，再 source scripts/steamdeck-env.sh 后运行。
独占自己启动的 OpenOCD；只进入 HOME/DEVICE/STATION，shell 白名单固定。
协议编码和 CRC 直接使用临时编译的仓库 protocol/vc_protocol.c。
"""
import argparse
import ctypes
import json
import math
import os
from pathlib import Path
import socket
import statistics
import struct
import subprocess
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
METRICS = ("loop_gap touch_gap gui_gap background gui_work input_mark shell_wait shell_exec "
           "image_wait image_work tx_wait_link tx_wait_station tx_wait_env "
           "tx_hold_link tx_hold_station tx_hold_env controlled_work diagnostic_step").split()
FLOW = "beep_high beep_drop shell_busy result_overwrite image_busy mouse_high inject_high".split()
NAMES = ("g_perf_stats g_perf_flow g_perf_reset g_perf_epoch g_perf_input_request "
         "g_perf_work_ms g_perf_work_active g_gui_inject g_gui_title g_reset_flags "
         "g_mouse_dropped g_lv_mem_used g_lv_mem_peak s_dropped uwTick").split()


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


class Probe:
    def __init__(self, folder, tcl_port, rtt_port, library):
        self.folder, self.tcl_port, self.rtt_port = folder, tcl_port, rtt_port
        self.proc = self.tcl = self.rtt = None
        self.buffer = bytearray()
        self.pending, self.done = {}, {}
        self.seq, self.telemetry = 0, 0
        self.telemetry_enabled = False
        self.log = (folder / "openocd.log").open("w")
        found = {}
        elf = ROOT / "firmware/station/build/Release/station.elf"
        for line in subprocess.check_output(["arm-none-eabi-nm", str(elf)], text=True).splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[2] in NAMES:
                require(parts[2] not in found, f"ambiguous symbol {parts[2]}")
                found[parts[2]] = int(parts[0], 16)
        require(set(found) == set(NAMES), f"missing symbols {set(NAMES) - set(found)}")
        self.symbols = found
        self.library = ctypes.CDLL(str(library))
        self.library.vc_encode.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint8,
                                          ctypes.c_uint8, ctypes.c_void_p, ctypes.c_uint16]
        self.library.vc_encode.restype = ctypes.c_size_t
        self.library.vc_crc8.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        self.library.vc_crc8.restype = ctypes.c_uint8
        self.library.vc_crc16.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_uint16]
        self.library.vc_crc16.restype = ctypes.c_uint16

    def start(self):
        for port in (self.tcl_port, self.rtt_port):
            with socket.socket() as test:
                test.bind(("127.0.0.1", port))
        self.proc = subprocess.Popen([
            "openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg",
            "-c", "gdb_port disabled", "-c", "telnet_port disabled",
            "-c", f"tcl_port {self.tcl_port}", "-c", "init"],
            stdout=self.log, stderr=self.log)
        deadline = time.monotonic() + 10
        while self.tcl is None:
            require(self.proc.poll() is None, "OpenOCD exited; see openocd.log")
            try:
                self.tcl = socket.create_connection(("127.0.0.1", self.tcl_port), .2)
                self.tcl.settimeout(8)
            except OSError:
                require(time.monotonic() < deadline, "TCL connection timed out")
                time.sleep(.1)
        self.cmd("reset run")
        time.sleep(4)
        self.cmd('rtt setup 0x20000000 0x20000 {SEGGER RTT}')
        self.cmd("rtt polling_interval 1")
        self.cmd("rtt start")
        self.cmd(f"rtt server start {self.rtt_port} 0")
        self.rtt = socket.create_connection(("127.0.0.1", self.rtt_port), 3)
        # 小帧并发时禁用客户端 Nagle，减少主机 TCP 合包等待对端到端测量的影响。
        self.rtt.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.rtt.settimeout(.02)
        self.request(5, struct.pack("<H", 50))
        self.telemetry_enabled = True
        # 空白处唤醒，再进入设备页；没有任何存储页面或命令。
        self.tap(470, 780)
        self.tap(392, 190)
        require(self.title() == "DEVICE", "not DEVICE")

    def close(self):
        if self.rtt and self.telemetry_enabled:
            try:
                self.request(5, b"\0\0")
            except (OSError, RuntimeError):
                # 连接已坏时无法保证发出停止命令，保留日志供排查。
                pass
        for sock in (self.rtt, self.tcl):
            if sock:
                sock.close()
        if self.proc and self.proc.poll() is None:
            self.proc.terminate()
            try:
                self.proc.wait(3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        self.log.close()

    def cmd(self, text):
        self.tcl.sendall(text.encode() + b"\x1a")
        out = bytearray()
        while not out.endswith(b"\x1a"):
            chunk = self.tcl.recv(65536)
            require(chunk, "TCL disconnected")
            out.extend(chunk)
        result = out[:-1].decode()
        require("Error:" not in result, result)
        return result.strip()

    def read(self, name, count=1, offset=0, bits=32):
        data = self.cmd(f"read_memory {self.symbols[name] + offset:#x} {bits} {count}")
        return [int(v, 0) for v in data.split()]

    def write(self, name, value, offset=0):
        self.cmd(f"mww {self.symbols[name] + offset:#x} {value}")

    def title(self):
        addr = self.read("g_gui_title")[0]
        data = self.cmd(f"read_memory {addr:#x} 8 16")
        return bytes(int(v, 0) for v in data.split()).split(b"\0")[0].decode()

    def tap(self, x, y):
        for kind in (0, 2):
            head = self.read("g_gui_inject")[0]
            addr = self.symbols["g_gui_inject"] + 8 + head % 8 * 8
            self.cmd(f"write_memory {addr:#x} 16 {{{kind} {x} {y}}}")
            self.write("g_gui_inject", head + 1)
            self.pump(.15 if kind == 0 else .6)

    def send(self, kind, payload=b""):
        self.seq = (self.seq + 1) % 256
        key = kind | 128, self.seq
        require(key not in self.pending and key not in self.done, "sequence reused before collection")
        frame = ctypes.create_string_buffer(1034)
        n = self.library.vc_encode(frame, 1034, kind, self.seq, payload, len(payload))
        require(n > 0, "encode failed")
        self.pending[key] = time.monotonic()
        self.rtt.sendall(frame.raw[:n])
        return key

    def pump(self, duration=0):
        deadline = time.monotonic() + duration
        while True:
            try:
                chunk = self.rtt.recv(65536)
                require(chunk, "RTT disconnected")
                self.buffer.extend(chunk)
            except socket.timeout:
                pass
            while len(self.buffer) >= 8:
                require(self.buffer[:2] == b"\xa5\x5a", "bad frame sync")
                header = bytes(self.buffer[2:7])
                require(self.library.vc_crc8(header, 5) == self.buffer[7], "bad header CRC")
                n = struct.unpack_from("<H", self.buffer, 5)[0]
                require(n <= 1024, "bad frame size")
                if len(self.buffer) < n + 10:
                    break
                frame = bytes(self.buffer[:n + 10])
                del self.buffer[:n + 10]
                crc = self.library.vc_crc16(frame[2:-2], len(frame) - 4, 65535)
                require(crc == struct.unpack_from("<H", frame, len(frame) - 2)[0], "bad body CRC")
                kind, seq = frame[3], frame[4]
                payload = frame[8:-2]
                if kind == 0x60:
                    self.telemetry += 1
                if kind & 128:
                    key = kind, seq
                    require(key in self.pending, f"unexpected/duplicate reply {key}")
                    elapsed = (time.monotonic() - self.pending.pop(key)) * 1000
                    self.done[key] = payload, elapsed
            if time.monotonic() >= deadline:
                break

    def collect(self, key, timeout=3):
        deadline = time.monotonic() + timeout
        while key not in self.done:
            require(time.monotonic() < deadline, f"response timed out: {key}")
            self.pump()
        return self.done.pop(key)

    def request(self, kind, payload=b""):
        out, elapsed = self.collect(self.send(kind, payload))
        require(out and out[0] == 0, f"command {kind} failed: {out!r}")
        if kind == 1:
            require(out[1:] == payload, "PING payload mismatch")
        return out[1:], elapsed

    def mark(self):
        require(self.read("g_perf_input_request")[0] == 0, "previous marker pending")
        self.write("g_perf_input_request", 1)

    def reset_window(self):
        require(not self.pending and not self.done, "uncollected requests")
        epoch = self.read("g_perf_epoch")[0]
        self.write("g_perf_reset", 1)
        deadline = time.monotonic() + 2
        while self.read("g_perf_epoch")[0] == epoch:
            require(time.monotonic() < deadline, "metric reset timed out")
            self.pump(.02)

    def snapshot(self, name, latencies, ps, free):
        # 短暂停核取得一致快照；不在运行中的测量区间停核。
        self.cmd("halt")
        try:
            raw = self.read("g_perf_stats", len(METRICS) * 4)
            flow = self.read("g_perf_flow", len(FLOW))
            memory = {n: self.read(n)[0] for n in ("g_lv_mem_used", "g_lv_mem_peak", "g_mouse_dropped", "s_dropped", "g_reset_flags", "uwTick")}
        finally:
            self.cmd("resume")
        require(not (memory["g_reset_flags"] & (1 << 29)), "watchdog reset")
        require(memory["s_dropped"] == 0, "RTT bytes dropped")
        stats = {}
        for i, metric in enumerate(METRICS):
            count, last, peak, over = raw[i*4:i*4+4]
            stats[metric] = dict(count=count, last_ms=round(last/168000, 4),
                                 peak_ms=round(peak/168000, 4), over20ms=over)
        ping = dict(count=len(latencies), median_ms=round(statistics.median(latencies), 3),
                    p95_ms=round(sorted(latencies)[math.ceil(.95*len(latencies))-1], 3),
                    max_ms=round(max(latencies), 3))
        result = dict(phase=name, stats=stats, flow=dict(zip(FLOW, flow)), ping=ping,
                      telemetry=self.telemetry, ps=ps.decode(), free=free.decode(), memory=memory)
        print(json.dumps(result, ensure_ascii=False), flush=True)
        return result

    def suite(self):
        results = []
        for phase in ("idle-device", "shell-device", "image-station", "controlled-180ms"):
            print(f"==== {phase} ====", flush=True)
            if phase == "image-station":
                self.tap(40, 38)
                self.tap(100, 190)
                require(self.title() == "STATION", "not STATION")
            elif phase == "controlled-180ms":
                self.tap(40, 38)
                self.tap(392, 190)
                require(self.title() == "DEVICE", "not DEVICE")
            self.reset_window()
            self.telemetry = 0
            latencies = []
            if phase == "idle-device":
                for _ in range(30):
                    self.mark()
                    latencies.append(self.request(1, b"idle")[1])
                    self.pump(.08)
            elif phase == "shell-device":
                for command in (b"uptime", b"sensors", b"ps", b"free") * 6:
                    self.mark()
                    shell = self.send(11, command)
                    latencies.append(self.request(1, bytes(256))[1])
                    reply, _ = self.collect(shell)
                    require(reply[0] == 0, "shell failed")
                    self.pump(.08)
            elif phase == "image-station":
                pixels = struct.pack("<HHHH", 0xf800, 0x07e0, 0x001f, 0xffff) * 4800
                for _ in range(5):
                    self.request(7, struct.pack("<HH", 160, 120))
                    for offset in range(0, len(pixels), 4000):
                        self.mark()
                        pending = [self.send(8, struct.pack("<I", pos) + pixels[pos:pos+1000])
                                   for pos in range(offset, min(offset+4000, len(pixels)), 1000)]
                        shell = self.send(11, b"uptime")
                        latencies.append(self.request(1, bytes(256))[1])
                        for key in pending + [shell]:
                            reply, _ = self.collect(key)
                            require(reply[0] == 0, "image/concurrent shell failed")
                        self.pump(.02)
                    self.pump(.15)
            else:
                for _ in range(5):
                    self.write("g_perf_work_ms", 180)
                    deadline = time.monotonic() + 2
                    while not self.read("g_perf_work_active")[0]:
                        require(time.monotonic() < deadline, "controlled work did not start")
                    self.mark()
                    pending = [self.send(11, b"uptime") for _ in range(3)]
                    for _ in range(8):
                        latencies.append(self.request(1, b"during-station-work")[1])
                    statuses = [self.collect(key)[0][0] for key in pending]
                    require(sorted(statuses) == [0, 3, 3], f"unexpected single-slot results {statuses}")
                    self.pump(.2)
            self.pump(.15)
            # 确保设备页任务快照在本轮负载后刷新；STATION 用 ps 取得新栈余量。
            taskout, _ = self.request(11, b"ps")
            freeout, _ = self.request(11, b"free")
            (self.folder / f"{phase}-ps.txt").write_bytes(taskout)
            results.append(self.snapshot(phase, latencies, taskout, freeout))
        require(results[2]["stats"]["image_work"]["count"] == 5, "not all images consumed")
        controlled = results[3]
        require(controlled["stats"]["controlled_work"]["count"] == 5, "wrong work count")
        require(controlled["stats"]["input_mark"]["peak_ms"] > 100, "marker missed controlled stall")
        require(controlled["stats"]["gui_gap"]["peak_ms"] > 170, "GUI stall not observed")
        require(controlled["flow"]["shell_busy"] == 10, "BUSY count mismatch")
        self.request(5, b"\0\0")
        self.telemetry_enabled = False
        require(self.title() == "DEVICE", "final page not DEVICE")
        print("PASS: four non-storage phases; five RTT images; five bounded stalls; BUSY and recovery; final DEVICE", flush=True)
        return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--tcl-port", type=int, default=6679)
    parser.add_argument("--rtt-port", type=int, default=19079)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    # 临时共享库只用来复用协议实现，实验结束删除，不放进仓库。
    with tempfile.TemporaryDirectory(prefix="vc-response-") as tmp:
        lib = Path(tmp) / "protocol.so"
        sysroot = ["--sysroot=" + os.environ["VC_SYSROOT"]] if os.environ.get("VC_SYSROOT") else []
        subprocess.run(["cc", *sysroot, "-shared", "-fPIC", "-O2",
                        str(ROOT / "protocol/vc_protocol.c"), "-o", str(lib)], check=True)
        probe = Probe(args.output, args.tcl_port, args.rtt_port, lib)
        try:
            probe.start()
            results = probe.suite()
            (args.output / "results.json").write_text(json.dumps(results, ensure_ascii=False, indent=2) + "\n")
        finally:
            probe.close()


if __name__ == "__main__":
    main()
