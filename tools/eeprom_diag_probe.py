#!/usr/bin/env python3
"""EEPROM 末页自检实板验证。只进 DEVICE，保留全部 256 字节；不访问 SD。"""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time
from scope_link_probe import ScopeProbe
from response_probe import ROOT, require


def shell(p, command):
    out, _ = p.request(11, command.encode())
    return out.decode()


def dump(p, start=0, length=256):
    data = bytearray()
    for addr in range(start, start+length, 32):
        n = min(32, start+length-addr)
        for line in shell(p, f"ee {addr} {n}").splitlines():
            match = re.fullmatch(r"[0-9a-f]{2}:((?: [0-9a-f]{2})+)", line)
            require(match, f"invalid EEPROM dump: {line!r}")
            data.extend(bytes.fromhex(match[1]))
    require(len(data) == length, "short EEPROM dump")
    return bytes(data)


def wait_done(p):
    deadline = time.monotonic()+3
    while True:
        status = shell(p, "eetest")
        require("restore-failed" not in status, "restore failed; RAM backup retained; run eetest retry")
        if " done " in status:
            require("pending 0" in status and "error 0 restore_error 0" in status, status)
            return status
        require(time.monotonic() < deadline, "EEPROM diagnostic did not finish")
        p.pump(.015)


def run(p):
    before = dump(p)
    # 不落盘保存用户 EEPROM 内容；比较在内存完成，只打印公共诊断页原值。
    print("==== 1. Reserved page and command validation ====", flush=True)
    print("page 248..255 before:", before[248:].hex(" "), flush=True)
    require(" idle none " in shell(p, "eetest"), "diagnostic auto-started")
    require("usage:" in shell(p, "eetest unknown"), "bad action accepted")
    require("usage:" in shell(p, "eetest start extra"), "extra arguments accepted")
    require("no failed restore" in shell(p, "eetest retry"), "retry accepted without backup")
    print("PASS: no automatic writes; invalid actions rejected; no failed restore", flush=True)

    print("==== 2. Full patterns, busy and repeat ====", flush=True)
    for index in range(2):
        require(" running " in shell(p, "eetest start"), "start failed")
        require("busy" in shell(p, "eetest start"), "second start overwrote active test")
        status = wait_done(p)
        require(" done pass " in status and "patterns 4/4 restored 1" in status, status)
        require(dump(p, 248, 8) == before[248:], "reserved page not restored")
        print(status, flush=True)

    print("==== 3. Cancel restores original page ====", flush=True)
    shell(p, "eetest start")
    shell(p, "eetest cancel")
    status = wait_done(p)
    require(" done cancelled " in status, status)
    require(dump(p, 248, 8) == before[248:], "cancel did not preserve original page")
    print(status, flush=True)

    print("==== 4. Concurrent scope, telemetry, PING and input marks ====", flush=True)
    p.reset_window()
    shell(p, "eetest start")
    latencies = []
    for _ in range(3):
        p.mark()
        meta = p.begin(50000, 1024)
        latencies.append(p.request(1, b"EEPROM-and-scope")[1])
        p.samples(meta)
    p.request(15)
    status = wait_done(p)
    require(" done pass " in status, status)
    require(dump(p) == before, "EEPROM contents changed outside or inside reserved page")
    ps = shell(p, "ps").encode()
    free = shell(p, "free").encode()
    result = p.snapshot("eeprom-diagnostic-and-scope", latencies, ps, free)
    require(result["stats"]["eeprom_step"]["count"] == 11, "expected one backup, eight pattern steps and two restore steps")
    (p.folder/"results.json").write_text(json.dumps(result, indent=2)+"\n")
    print(status+"\n"+ps.decode()+"\n"+free.decode(), flush=True)
    print("PASS: full 256-byte EEPROM unchanged; four patterns twice, cancel, concurrent scope and telemetry; final DEVICE", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args(); args.output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="vc-ee-diag-") as tmp:
        lib = Path(tmp)/"protocol.so"
        flags = ["--sysroot="+os.environ["VC_SYSROOT"]] if os.environ.get("VC_SYSROOT") else []
        subprocess.run(["cc", *flags, "-shared", "-fPIC", str(ROOT/"protocol/vc_protocol.c"), "-o", str(lib)], check=True)
        p = ScopeProbe(args.output, 6679, 19079, lib)
        try:
            p.start(); run(p)
        finally:
            try:
                if p.rtt:
                    # 若测试中途抛错，仍尽力请求取消，让固件自行恢复诊断页。
                    status = shell(p, "eetest cancel")
                    if " running " in status:
                        wait_done(p)
                    require("restore-failed" not in status, "restore failed; backup retained; run eetest retry")
                    p.request(15)
            finally:
                p.close()


if __name__ == "__main__": main()
