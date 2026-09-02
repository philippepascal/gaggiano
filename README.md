# Gaggiano

Firmware for a Gaggia espresso machine retrofit, inspired by
[Gaggiuino](https://github.com/Zer0-bit/gaggiuino) but written from scratch around two
boards:

| Board | Sketch | Role |
|---|---|---|
| STM32F411CEU6 "Black Pill" | `gaggiano-controller-v1/` | reads boiler temperature (MAX6675) and pressure (ADS1115), drives the boiler SSR, the pump (pulse-skip modulation) and the 3-way solenoid |
| Sunton ESP32-8048S043 (ESP32-S3, 4.3" 800x480 touch) | `gaggiano-v2/` | LVGL touch UI, brew profiles and logs on SD card, sends setpoints to the controller over UART |

The two talk over a 115200 baud serial link with a small text protocol with checksums,
a 1 s heartbeat from the screen and a 3 s safety timeout on the controller; see
`docs/PROTOCOL.md`. The protocol code is shared by both firmwares and tested on the
host (`libraries/GaggiaProtocol/`, `tests/`). Wiring follows the Gaggiuino "Lego build"
schematic in `docs/images/gaggiuino-lego-schematic-v3.9.png` with pin assignments as in
the sketches (`gaggiano-controller-v1/config.h`, `gaggiano-v2/config.h`).

## Building and flashing

Everything goes through the `./gg` script (arduino-cli underneath, no Arduino IDE):

```
./gg setup                 # once: pin the cores, write the local config
./gg build all
./gg flash screen          # esptool over the display's USB port
./gg flash controller      # DFU over USB, no buttons needed after the first flash
./gg monitor controller
./gg test                  # host-side tests of the protocol, profile format, sequencer
```

Both boards answer commands on their USB console (`tools/serial-cmd.py controller STATUS`,
`tools/serial-cmd.py screen STATUS`; `--help` lists them). `docs/BENCH-CHECKLIST.md` is
the manual verification run after a change.

- `docs/BUILD.md`: setup, commands, layout, troubleshooting.
- `docs/FLASH-STM32.md`: how the STM32 gets into DFU, the serial-triggered reboot,
  the button sequence, the ST-Link alternative.
- `sdcard/`: what goes on the display's SD card.
- `docs/2026-09-01-findings.md` and `docs/MIGRATION-PLAN.md`: state of the project and
  the record of the move away from the Arduino IDE.
- `docs/REFACTOR-PLAN.md`: the bug inventory and the refactoring record (protocol v2,
  memory fixes, watchdog, tests).

## Layout

```
gg, tools/            build/flash tooling
gaggiano-controller-v1/, gaggiano-v2/   the two sketches
libraries/            vendored Arduino libraries (with the project lv_conf.h)
sdcard/               SD card content for the display
vendor/               display board datasheets and schematics
gaggiacover/          3D models for the screen enclosure
archive/              first version of the display firmware
docs/                 documentation and images
```

## Known issues and ideas

From `notes.txt`, the working list, updated after the 2026-09 refactor:

- Communication stopping: the screen leaked heap on every status line and ran out
  within minutes; fixed (see `docs/REFACTOR-PLAN.md`, B1). To be confirmed in the machine.
- Pressure readout noisy on the screen: the reader now drains every pending line and
  shows the newest at 5 Hz; smoothing strategy still open.
- Boiler PID does not settle cleanly: zero readings from the MAX6675 used to pass the
  range check and reach the PID; fixed (B2). Re-check in the machine before retuning.
- Steam pressure declines over a long steam; needs a strategy to hold some pressure.
- Full controller logs on the SD card would help tuning; log format needs a header and
  a way to tie settings to a log (hash of the settings).
- Blooming: the pre-infusion fill time may not be consistent between the first and
  subsequent shots; a "stop pump when water detected" command is a candidate fix.
