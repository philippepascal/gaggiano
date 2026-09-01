#!/usr/bin/env python3
"""Send one line to a serial port and print what comes back for a while.

usage: serial-cmd.py PORT LINE [SECONDS] [BAUD]
Uses only the standard library (termios), so it works with the system python3.
"""
import os
import select
import sys
import termios
import time


def main():
    port, line = sys.argv[1], sys.argv[2]
    seconds = float(sys.argv[3]) if len(sys.argv) > 3 else 2.0
    baud = getattr(termios, "B%s" % (sys.argv[4] if len(sys.argv) > 4 else "9600"))
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0                     # iflag: raw
        attrs[1] = 0                     # oflag: raw
        attrs[2] = termios.CS8 | termios.CREAD | termios.CLOCAL | termios.HUPCL
        attrs[3] = 0                     # lflag: raw
        attrs[4] = attrs[5] = baud
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        termios.tcflush(fd, termios.TCIFLUSH)
        time.sleep(0.1)
        os.write(fd, (line + "\n").encode())
        end = time.time() + seconds
        out = b""
        while time.time() < end:
            r, _, _ = select.select([fd], [], [], 0.1)
            if r:
                try:
                    out += os.read(fd, 4096)
                except OSError:
                    break                # port went away (e.g. device reset)
        sys.stdout.write(out.decode("utf-8", "replace"))
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
