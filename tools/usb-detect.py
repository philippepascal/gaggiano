#!/usr/bin/env python3
"""List USB devices relevant to the project and map them to serial ports.

Usage:
  usb-detect.py [--config-file F] SPEC... [--json | --port TARGET STATE | --present TARGET STATE]

SPEC is "target:state=vvvv:pppp" (hex vid:pid), e.g. "controller:dfu=0483:df11".
Default output is a human table. Exit status: 0 if at least one known device is
present (or, with --port/--present, if the requested one is), 1 otherwise.

Device presence comes from `ioreg` (works even when the device has no serial
port, e.g. the STM32 ROM bootloader). Port addresses come from
`arduino-cli board list`, matched by vid:pid and serial number.
"""
import json
import re
import subprocess
import sys


def ioreg_devices():
    """Return [{vid, pid, name, serial}] for every USB device ioreg knows."""
    out = subprocess.run(["ioreg", "-p", "IOUSB", "-l", "-w0"], capture_output=True, text=True).stdout
    devices, cur = [], None
    for line in out.splitlines():
        if "+-o " in line:
            if cur and "vid" in cur:
                devices.append(cur)
            cur = {"name": line.split("+-o ", 1)[1].split("  <", 1)[0].strip()}
            continue
        m = re.search(r'"([^"]+)" = (.*)$', line)
        if not m or cur is None:
            continue
        key, val = m.group(1), m.group(2).strip()
        if key == "idVendor":
            cur["vid"] = "%04x" % int(val)
        elif key == "idProduct":
            cur["pid"] = "%04x" % int(val)
        elif key == "USB Serial Number":
            cur["serial"] = val.strip('"')
        elif key == "USB Product Name":
            cur["name"] = val.strip('"')
    if cur and "vid" in cur:
        devices.append(cur)
    return devices


def cli_ports(config_file):
    """Return {(vid, pid, serial): address} from arduino-cli board list."""
    cmd = ["arduino-cli"]
    if config_file:
        cmd += ["--config-file", config_file]
    cmd += ["board", "list", "--format", "json"]
    try:
        data = json.loads(subprocess.run(cmd, capture_output=True, text=True, timeout=20).stdout or "{}")
    except (subprocess.TimeoutExpired, json.JSONDecodeError):
        return {}
    ports = {}
    for entry in data.get("detected_ports", []):
        port = entry.get("port", {})
        props = port.get("properties", {}) or {}
        vid, pid = props.get("vid", "").lower().replace("0x", ""), props.get("pid", "").lower().replace("0x", "")
        if vid and pid:
            ports[(vid, pid, props.get("serialNumber", ""))] = port.get("address")
    return ports


def main(argv):
    config_file, mode, want = None, "table", None
    specs = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--config-file":
            config_file = argv[i + 1]; i += 2
        elif a == "--json":
            mode = "json"; i += 1
        elif a in ("--port", "--present"):
            mode = a[2:]; want = (argv[i + 1], argv[i + 2]); i += 3
        else:
            specs.append(a); i += 1

    table = {}  # (vid, pid) -> (target, state)
    for s in specs:
        m = re.match(r"^([\w-]+):([\w-]+)=([0-9a-fA-F]{4}):([0-9a-fA-F]{4})$", s)
        if not m:
            sys.exit("bad spec: %s" % s)
        table[(m.group(3).lower(), m.group(4).lower())] = (m.group(1), m.group(2))

    ports = cli_ports(config_file)
    found = []
    for d in ioreg_devices():
        key = (d["vid"], d["pid"])
        if key not in table:
            continue
        target, state = table[key]
        addr = ports.get((d["vid"], d["pid"], d.get("serial", "")))
        if addr is None:  # serial number formats can differ; fall back to vid:pid only
            for (v, p, _s), a in ports.items():
                if (v, p) == key:
                    addr = a
        found.append({"target": target, "state": state, "vid": d["vid"], "pid": d["pid"],
                      "name": d.get("name", ""), "serial": d.get("serial", ""), "port": addr})

    if mode == "json":
        print(json.dumps(found, indent=2))
        return 0 if found else 1
    if mode in ("port", "present"):
        for f in found:
            if (f["target"], f["state"]) == want:
                if mode == "port":
                    if f["port"]:
                        print(f["port"])
                        return 0
                    continue
                return 0
        return 1
    if not found:
        print("no known device connected")
        return 1
    w = max(len(f["target"]) for f in found)
    for f in found:
        print("%-*s  %-7s %s:%s  %-28s %s" % (w, f["target"], f["state"], f["vid"], f["pid"],
                                             f["name"][:28], f["port"] or "(no serial port)"))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
