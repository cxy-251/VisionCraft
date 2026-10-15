#!/usr/bin/env python3
"""A05 正式协议与 PA4 实板回环。无接线、无存储访问；退出时停止采集和遥测。"""
import argparse
import json
from pathlib import Path
import os
import socket
import struct
import subprocess
import tempfile
import time
from response_probe import Probe, ROOT, require
from scope_analyze import frequency


class ScopeProbe(Probe):
    def add_symbols(self):
        names = {"g_scope_state", "g_scope_completed", "g_scope_errors"}
        elf = ROOT / "firmware/station/build/Release/station.elf"
        for line in subprocess.check_output(["arm-none-eabi-nm", str(elf)], text=True).splitlines():
            fields = line.split()
            if len(fields) == 3 and fields[2] in names:
                self.symbols[fields[2]] = int(fields[0], 16)
        require(names <= self.symbols.keys(), "scope symbols missing")

    def status(self, command, payload, expected):
        out, _ = self.collect(self.send(command, payload))
        require(out and out[0] == expected, f"command {command}: {out!r}, expected {expected}")

    def begin(self, rate, count):
        out, _ = self.request(13, struct.pack("<IH", rate, count))
        require(len(out) == 20, "metadata length")
        meta = struct.unpack("<IIIIHH", out)
        ident, clock, adcdiv, dacdiv, total, vdda = meta
        require(ident and clock == 84000000 and total == count and adcdiv and dacdiv, "invalid metadata")
        require(abs(clock/adcdiv-rate) <= rate*.002, "actual ADC rate mismatch")
        return meta

    def samples(self, meta):
        ident, clock, adcdiv, dacdiv, total, vdda = meta
        values = []
        busy = 0
        deadline = time.monotonic()+3
        while len(values) < total:
            count = min(256, total-len(values))
            out, _ = self.collect(self.send(14, struct.pack("<IHH", ident, len(values), count)))
            if out[0] == 3:
                busy += 1
                require(time.monotonic() < deadline, "DMA capture never became ready")
                self.pump(.005)
                continue
            require(out[0] == 0 and len(out) == 1+8+2*count, f"bad sample response {out[:10]!r}")
            require(struct.unpack("<IHH", out[1:9]) == (ident, len(values), total), "block identity/offset/total mismatch")
            values.extend(struct.unpack("<"+"H"*count, out[9:]))
        require(all(0 <= x <= 4095 for x in values), "sample outside ADC range")
        mean, crossings, hz = frequency(values, clock/adcdiv)
        require(hz and abs(hz-clock/dacdiv/128)/(clock/dacdiv/128) < .02, "not the generated sine frequency")
        require(min(values) < 800 and max(values) > 3200, "loopback amplitude missing")
        return values, dict(id=ident, rate=clock/adcdiv, count=total, dac_hz=clock/dacdiv/128,
            min=min(values), max=max(values), mean=round(mean, 2), measured_hz=round(hz, 3), busy=busy, vdda_mv=vdda)

    def stopped(self):
        for address in (0x40000000, 0x40001000, 0x40007400, 0x40012108, 0x40026088, 0x40026440):
            value = int(self.cmd(f"read_memory {address:#x} 32 1"), 0)
            require(not (value & 1), f"hardware still enabled {address:#x}: {value:#x}")

    def suite(self):
        self.add_symbols()
        print("==== 1. Historical register scripts protected ====", flush=True)
        apb = int(self.cmd("read_memory 0x40023840 32 1"), 0)
        require(apb & (1 << 29), "DAC ownership clock missing")
        for script in ("scope_probe.tcl", "dac_probe.tcl"):
            msg = self.cmd(f"catch {{source {ROOT / 'tools' / script}}} message; set message")
            require("DAC 已" in msg, "legacy script did not refuse")
            print(script+": "+msg, flush=True)
        print("==== 2. Invalid arguments and capture BUSY ====", flush=True)
        for rate, count in ((9999, 2048), (200001, 2048), (50000, 0), (50000, 2049)):
            self.status(13, struct.pack("<IH", rate, count), 1)
        self.status(13, b"", 1)
        self.status(15, b"x", 1)
        meta = self.begin(10000, 2048)
        self.status(13, struct.pack("<IH", 50000, 2048), 3)
        self.status(14, struct.pack("<IHH", meta[0], 0, 256), 3)
        self.status(14, struct.pack("<IHH", meta[0], 2040, 32), 1)
        self.status(14, struct.pack("<IHH", meta[0], 0, 257), 1)
        self.request(15)
        self.stopped()
        self.status(14, struct.pack("<IHH", meta[0], 0, 256), 4)
        print("PASS: range/length, BUSY, bounds, STOP releases hardware and buffer", flush=True)
        print("==== 3. Three sample rates ====", flush=True)
        summaries = []
        for rate in (10000, 50000, 200000):
            values, summary = self.samples(self.begin(rate, 2048))
            self.stopped()  # DMA 完成后，无须等上位机 STOP 就自动停发生器
            (self.folder/f"samples-{rate}.txt").write_text("\n".join(map(str, values))+"\n")
            summaries.append(summary)
            print(json.dumps(summary), flush=True)
        print("==== 4. Client loss and stale capture IDs ====", flush=True)
        meta = self.begin(10000, 2048)
        self.rtt.close()
        time.sleep(.4)
        self.stopped()
        self.rtt = socket.create_connection(("127.0.0.1", self.rtt_port), 3)
        self.rtt.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        self.rtt.settimeout(.02)
        self.buffer.clear()
        newer = self.begin(50000, 512)
        require(newer[0] != meta[0], "capture ID did not change")
        self.status(14, struct.pack("<IHH", meta[0], 0, 256), 1)
        self.samples(newer)
        for _ in range(3): self.request(15)
        print("PASS: finite capture stopped without client, reconnect/restart rejects stale ID, repeated STOP", flush=True)
        print("==== 5. Twenty frames with telemetry, PING and input marks ====", flush=True)
        self.reset_window()
        latency = []
        for _ in range(20):
            self.mark()
            meta = self.begin(50000, 1024)
            latency.append(self.request(1, b"scope-concurrent-ping")[1])
            self.samples(meta)
        self.request(15)
        ps, _ = self.request(11, b"ps")
        free, _ = self.request(11, b"free")
        result = self.snapshot("scope-20-frames", latency, ps, free)
        result["captures"] = summaries
        result["completed"] = self.read("g_scope_completed")[0]
        result["errors"] = self.read("g_scope_errors")[0]
        require(result["errors"] == 0, "scope DMA errors")
        self.stopped()
        print(ps.decode()+"\n"+free.decode(), flush=True)
        print(f"completed={result['completed']} errors={result['errors']} state={self.read('g_scope_state')[0]}", flush=True)
        (self.folder / "results.json").write_text(json.dumps(result, indent=2)+"\n")
        print("PASS: actual PA4 loopback, no wires/storage, normal firmware remains responsive; final DEVICE", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vc-scope-") as tmp:
        lib = Path(tmp)/"protocol.so"
        flags = ["--sysroot="+os.environ["VC_SYSROOT"]] if os.environ.get("VC_SYSROOT") else []
        subprocess.run(["cc", *flags, "-shared", "-fPIC", "-O2", str(ROOT/"protocol/vc_protocol.c"), "-o", str(lib)], check=True)
        p = ScopeProbe(args.output, 6679, 19079, lib)
        try:
            p.start(); p.suite()
        finally:
            try:
                if p.rtt: p.request(15)
            finally:
                p.close()


if __name__ == "__main__": main()
