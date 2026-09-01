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
5. It downloads the binary with STM32CubeProgrammer, retrying up to 3 times, then
   waits for the firmware's serial port to come back.

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
- macOS occasionally needs a second attempt; `gg` retries automatically.
- After a failed download the bootloader may still be active. Just run
  `./gg flash controller --no-build` again.

## Serial-triggered DFU (no buttons)

The firmware accepts the line `DFU` on its USB serial port (the 9600 baud CDC port,
not the screen link). It zeroes all outputs, stores a marker in an RTC backup register
and resets; on the next boot the marker is detected before anything else runs and
execution jumps to the ROM bootloader. `./gg flash controller` uses this automatically;
`--no-serial-dfu` skips it. Verified on hardware on 2026-09-01.

Manual use: `tools/serial-cmd.py /dev/cu.usbmodemXXXX DFU` (a bare shell redirect tends to
close the port before the bytes leave the Mac). `tools/serial-cmd.py <port> VERSION` prints
the firmware version string.

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
| `Target device not found` from CubeProgrammer | board not in DFU; redo the button sequence, check cable/port, relay quirk above |
| DFU seen but download fails at a sector | run again with `--no-build`; if it persists, erase first with `STM32_Programmer_CLI --connect port=usb1 --erase all` |
| Board silent after flash | the build must have `usb=CDCgen` (it does, in `tools/targets.sh`); press NRST once |
| `STM32CubeProgrammer CLI not found` | install it from ST's site; `gg` looks in `/Applications/STMicroelectronics/...` and on `PATH` |
| Two `usbmodem` ports | `./gg detect` shows which one is the controller; pass `--port` to `monitor` |
