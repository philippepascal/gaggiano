#!/usr/bin/env python3
"""Send one line to a serial port and print what comes back.

Standard library only (termios), so it works with the system python3.
"""
import argparse
import glob
import json
import os
import select
import subprocess
import sys
import termios
import time

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TARGET_BAUD = {"controller": 9600, "screen": 115200}

EPILOG = """\
port:
  a device path such as /dev/cu.usbmodem3367335531341 (controller, USB CDC) or
  /dev/cu.usbserial-11140 (screen, CH340), or the words "controller" / "screen",
  which are resolved with ./gg detect and pick the right baud rate.
  On macOS always use the cu.* name, never tty.*.

examples:
  %(prog)s --list                          show serial ports and what they are
  %(prog)s controller VERSION              ask the controller for its version
  %(prog)s controller STATUS
  %(prog)s controller "RX 1;0;0;|"         inject a screen-protocol line
  %(prog)s controller "LOG ON" -t 5        turn logging on and watch 5 s of output
  %(prog)s screen -t 15                    just listen to the screen for 15 s
  %(prog)s /dev/cu.usbmodem3367335531341 DFU

controller commands: VERSION, STATUS, LOG ON, LOG OFF, RX <line>, HANG, DFU
(see docs/BENCH-CHECKLIST.md).
"""


def list_ports():
    """Print every cu.* port with vid:pid and, when known, the project target."""
    ports = [p for p in sorted(glob.glob("/dev/cu.*"))
             if "Bluetooth" not in p and "debug-console" not in p]
    info = {}
    try:
        out = subprocess.run([os.path.join(REPO, "gg"), "detect", "--json"],
                             capture_output=True, text=True, timeout=30).stdout
        for d in json.loads(out or "[]"):
            if d.get("port"):
                info[d["port"]] = "%s (%s) %s:%s %s" % (d["target"], d["state"], d["vid"], d["pid"], d["name"])
    except (OSError, ValueError, subprocess.TimeoutExpired):
        pass
    if not ports:
        print("no USB serial ports found")
        return 1
    for p in ports:
        print("%-36s %s" % (p, info.get(p, "")))
    return 0


def resolve_port(name):
    """'controller' / 'screen' -> (path, baud); a path -> (path, None)."""
    if name not in TARGET_BAUD:
        return name, None
    try:
        out = subprocess.run([os.path.join(REPO, "gg"), "detect", "--json"],
                             capture_output=True, text=True, timeout=30).stdout
        for d in json.loads(out or "[]"):
            if d["target"] == name and d["state"] == "run" and d.get("port"):
                return d["port"], TARGET_BAUD[name]
    except (OSError, ValueError, subprocess.TimeoutExpired):
        pass
    sys.exit("error: no %s found in run mode (try --list, or ./gg detect)" % name)


def talk(port, line, seconds, baud):
    try:
        fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    except OSError as e:
        sys.exit("error: cannot open %s: %s (try --list)" % (port, e.strerror))
    try:
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0                     # iflag: raw
        attrs[1] = 0                     # oflag: raw
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL | termios.HUPCL
        attrs[3] = 0                     # lflag: raw
        attrs[4] = attrs[5] = getattr(termios, "B%d" % baud)
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        termios.tcflush(fd, termios.TCIFLUSH)
        time.sleep(0.1)
        if line is not None:
            os.write(fd, (line + "\n").encode())
        end = time.time() + seconds
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    data = os.read(fd, 4096)
                except OSError:
                    print("(port closed: device reset or unplugged)")
                    break
                sys.stdout.write(data.decode("utf-8", "replace"))
                sys.stdout.flush()
    finally:
        os.close(fd)


def main():
    ap = argparse.ArgumentParser(
        description="Send one line to a serial port and print the reply.",
        epilog=EPILOG, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("port", nargs="?", help="device path, or controller / screen")
    ap.add_argument("line", nargs="?", help="text to send (a newline is added); omit to only listen")
    ap.add_argument("-l", "--list", action="store_true", help="list serial ports and exit")
    ap.add_argument("-t", "--time", type=float, default=2.0, metavar="SECONDS",
                    help="how long to print incoming data (default 2)")
    ap.add_argument("-b", "--baud", type=int, metavar="BAUD",
                    help="baud rate (default: 9600 for controller, 115200 for screen or a raw path)")
    args = ap.parse_args()

    if args.list:
        sys.exit(list_ports())
    if not args.port:
        ap.print_help()
        sys.exit(2)
    port, baud = resolve_port(args.port)
    baud = args.baud or baud or 115200
    talk(port, args.line, args.time, baud)


if __name__ == "__main__":
    main()
