#!/usr/bin/env python3
"""A04 实板回归：会设置测试日历，结束时恢复电脑本地时间；不访问 SD 卡。
source scripts/steamdeck-env.sh 后，python3 tools/clock_probe.py --output /tmp/vc-clock
"""
import argparse
from datetime import datetime
import os
from pathlib import Path
import socket
import struct
import subprocess
import tempfile
import time
from response_probe import Probe, ROOT, require


def encode_time(t):
    return struct.pack("<H6B", t.year, t.month, t.day, t.hour, t.minute, t.second, t.isoweekday())


def read_time(p):
    data, _ = p.request(12)
    require(len(data) == 10 and data[0] in (0, 1) and data[1] == 1, "invalid RTC reply")
    if not data[0]:
        require(data[2:] == bytes(8), "invalid clock must have zero calendar")
        return None
    year, month, day, hour, minute, second, week = struct.unpack("<H6B", data[2:])
    t = datetime(year, month, day, hour, minute, second)
    require(week == t.isoweekday(), "RTC weekday mismatch")
    return t


def run(p):
    initial = read_time(p)
    print(f"==== 1. Initial RTC: {initial} ====", flush=True)
    # 模拟未校时，仅清本功能的 DR0 标记；不复位整个备份域，不碰其他备份寄存器。
    p.cmd("mww 0x40002850 0")
    require(read_time(p) is None, "missing invalid-time state")
    print("DR0=0 -> valid=0 source=LSE calendar=zeros", flush=True)

    print("==== 2. Reject invalid input, preserve valid calendar ====", flush=True)
    baseline = datetime(2026, 10, 15, 12, 34, 56)
    p.request(3, encode_time(baseline))
    bad = [encode_time(baseline)[:7], struct.pack("<H6B", 2026, 2, 29, 0, 0, 0, 7),
           struct.pack("<H6B", 2026, 4, 31, 0, 0, 0, 4),
           struct.pack("<H6B", 2100, 1, 1, 0, 0, 0, 5),
           struct.pack("<H6B", 2026, 10, 15, 24, 0, 0, 4),
           struct.pack("<H6B", 2026, 10, 15, 12, 34, 56, 5)]
    for payload in bad:
        out, _ = p.collect(p.send(3, payload))
        require(out == b"\1", f"invalid SET_TIME accepted {payload.hex()}")
        print(f"SET_TIME {payload.hex()} -> ARGS", flush=True)
    out, _ = p.collect(p.send(12, b"x"))
    require(out == b"\1", "GET_TIME with payload accepted")
    after = read_time(p)
    require(0 <= (after - baseline).total_seconds() <= 2, "bad request changed RTC")
    print(f"GET_TIME bad length -> ARGS; retained={after}", flush=True)

    print("==== 3. Leap day, month/year rollover ====", flush=True)
    for start, expected in [(datetime(2024, 2, 28, 23, 59, 59), (2024, 2, 29)),
                            (datetime(2025, 2, 28, 23, 59, 59), (2025, 3, 1)),
                            (datetime(2026, 12, 31, 23, 59, 59), (2027, 1, 1))]:
        p.request(3, encode_time(start))
        p.pump(2.2)
        actual = read_time(p)
        require((actual.year, actual.month, actual.day) == expected, "calendar rollover failed")
        require(1 <= (actual - start).total_seconds() <= 4, "RTC not ticking")
        print(f"{start} -> {actual} weekday={actual.isoweekday()}", flush=True)

    print("==== 4. Reset retention and RTT reconnection ====", flush=True)
    p.request(3, encode_time(datetime(2028, 2, 29, 12, 0, 0)))
    before = read_time(p)
    p.cmd("reset run")
    p.pump(4)
    after = read_time(p)
    require(2 <= (after - before).total_seconds() <= 6, "reset lost calendar")
    p.telemetry_enabled = False  # 复位后固件默认不订阅
    print(f"reset: {before} -> {after}", flush=True)
    p.rtt.close()
    p.rtt = socket.create_connection(("127.0.0.1", p.rtt_port), 3)
    p.rtt.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    p.rtt.settimeout(.02)
    p.pump(1.1)
    reconnect = read_time(p)
    require(1 <= (reconnect - after).total_seconds() <= 3, "reconnection lost calendar")
    print(f"reconnect: {reconnect}", flush=True)
    for _ in range(40):
        require(read_time(p) is not None, "read failure after repetition")
    p.tap(470, 780)
    p.tap(392, 190)
    require(p.title() == "DEVICE", "final page not DEVICE")
    p.pump(1.2)
    print("==== 5. Resource measurements ====", flush=True)
    for cmd in (b"ps", b"free"):
        out, _ = p.request(11, cmd)
        print(out.decode(), flush=True)
    print(f"reset_flags={p.read('g_reset_flags')[0]:#x}", flush=True)
    require(not (p.read("g_reset_flags")[0] & (1 << 29)), "watchdog reset")
    print("PASS: invalid state/arguments, Gregorian rollovers, ticking, reset retention, RTT reconnection, 40 reads; no storage", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vc-clock-") as tmp:
        lib = Path(tmp) / "protocol.so"
        flags = ["--sysroot=" + os.environ["VC_SYSROOT"]] if os.environ.get("VC_SYSROOT") else []
        subprocess.run(["cc", *flags, "-shared", "-fPIC", "-O2", str(ROOT / "protocol/vc_protocol.c"), "-o", str(lib)], check=True)
        p = Probe(args.output, 6679, 19079, lib)
        try:
            p.start()
            run(p)
        finally:
            try:
                if p.rtt:
                    now = datetime.now().replace(microsecond=0)
                    p.request(3, encode_time(now))
                    actual = read_time(p)
                    require(abs((actual - datetime.now()).total_seconds()) < 3, "final time readback differs from host")
                    print(f"RESTORED local host time: {actual}", flush=True)
            finally:
                p.close()


if __name__ == "__main__":
    main()
