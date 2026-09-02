# Flashing the STM32 controller

The controller is an STM32F411CEU6 "Black Pill". It is programmed over USB using the
chip's ROM bootloader (DFU). The bootloader lives in ROM, so no firmware bug can brick
the board: the button sequence below always works.

## Three USB identities

| State | VID:PID | What macOS shows | How to see it |
|---|---|---|---|
| Running our firmware | 0483:5740 | `/dev/cu.usbmodem<serial>` | `./gg detect`, `ls /dev/cu.*` |
| DFU bootloader | 0483:df11 | **nothing under /dev** | `./gg detect`, `dfu-util -l` |
| ST-Link probe (optional) | 0483:3748 or 0483:374b | nothing under /dev | `st-info --probe` |

The DFU bootloader is a plain USB device, not a serial port. This is why the Arduino IDE
port menu looked empty even when the board was correctly in DFU mode.

## Normal flash

```
./gg flash controller
```

What happens:

1. The sketch is built into `build/controller/`.
2. If the board is already in DFU mode, go to step 5.
3. If the board is running firmware that understands the `DFU` serial command (see
   below), `gg` sends it and waits up to 5 s for the bootloader to appear.
4. Otherwise it prints the button sequence and polls for the bootloader for 60 s
   (`--timeout` to change).
5. It downloads the binary with `dfu-util` (Homebrew, open source; ST's
   STM32CubeProgrammer is not needed, `--via cubeprog` uses it if installed), retrying
   up to 3 times, then waits for the firmware's serial port to come back.

## The button sequence

On the Black Pill (pinout picture: `gaggiano-controller-v1/Black_pill_pinout.png`):

1. Hold **BOOT0**.
2. Press and release **NRST**.
3. Release **BOOT0**.

Within a second `./gg detect` should list `controller dfu 0483:df11 STM32 BOOTLOADER`.

Known quirks, from experience with this build:

- The solenoid relay can prevent DFU enumeration. Turning brew on from the screen
  before entering DFU has helped.
- Use a direct USB port on the Mac rather than a hub, and a data-capable cable.
  Charge-only cables power the board but the Mac never sees it. The WeAct v2.0
  Black Pill has no CC resistors on its USB-C socket, so a C-to-C cable from a Mac
  may not enumerate at all; use a USB-A to USB-C cable. Label the cable that works.
- macOS occasionally needs a second attempt; `gg` retries automatically.
- After a failed download the bootloader may still be active. Just run
  `./gg flash controller --no-build` again.

## Serial-triggered DFU (no buttons)

The firmware accepts the line `DFU` on its USB serial port (the 9600 baud CDC port,
not the screen link). It zeroes all outputs, stores a marker in an RTC backup register
and resets; on the next boot the marker is detected before anything else runs and
execution jumps to the ROM bootloader. `./gg flash controller` uses this automatically;
`--no-serial-dfu` skips it. Verified on hardware on 2026-09-01.

Manual use: `tools/serial-cmd.py controller DFU` (a bare shell redirect tends to close the
port before the bytes leave the Mac). `tools/serial-cmd.py controller VERSION` prints the
firmware version string; `tools/serial-cmd.py --list` shows the ports; `--help` for more.

## Alternative: ST-Link over SWD

With an ST-Link (or clone) wired to the SWD header (SWDIO, SWCLK, GND, 3V3), the
bootloader is not needed at all:

```
brew install stlink
./gg flash controller --via stlink
```

This also enables debugging with gdb and works even when USB is dead.

## Troubleshooting

| Symptom | Check |
|---|---|
| `No DFU capable USB device available` from dfu-util | board not in DFU; redo the button sequence, check cable/port, relay quirk above |
| DFU seen but download fails at a sector | run again with `--no-build`; if it persists, erase first with `dfu-util -d 0483:df11 -a 0 -s 0x08000000 -D /dev/zero` is not needed; use `--via cubeprog` with STM32CubeProgrammer as a second opinion |
| Board silent after flash | the build must have `usb=CDCgen` (it does, in `tools/targets.sh`); press NRST once |
| `dfu-util not found` | `brew install dfu-util` (`./gg setup` does it) |
| Two `usbmodem` ports | `./gg detect` shows which one is the controller; pass `--port` to `monitor` |
