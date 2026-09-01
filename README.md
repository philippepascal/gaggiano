# Gaggiano

Firmware for a Gaggia espresso machine retrofit, inspired by
[Gaggiuino](https://github.com/Zer0-bit/gaggiuino) but written from scratch around two
boards:

| Board | Sketch | Role |
|---|---|---|
| STM32F411CEU6 "Black Pill" | `gaggiano-controller-v1/` | reads boiler temperature (MAX6675) and pressure (ADS1115), drives the boiler SSR, the pump (pulse-skip modulation) and the 3-way solenoid |
| Sunton ESP32-8048S043 (ESP32-S3, 4.3" 800x480 touch) | `gaggiano-v2/` | LVGL touch UI, brew profiles and logs on SD card, sends setpoints to the controller over UART |

The two talk over a 115200 baud serial link with a small ASCII protocol
(`type;field;field;...|`). Wiring follows the Gaggiuino "Lego build" schematic in
`docs/images/gaggiuino-lego-schematic-v3.9.png` with pin assignments as in the sketches.

## Building and flashing

Everything goes through the `./gg` script (arduino-cli underneath, no Arduino IDE):

```
./gg setup                 # once: pin the cores, write the local config
./gg build all
./gg flash screen          # esptool over the display's USB port
./gg flash controller      # DFU over USB, no buttons needed after the first flash
./gg monitor controller
```

- `docs/BUILD.md`: setup, commands, layout, troubleshooting.
- `docs/FLASH-STM32.md`: how the STM32 gets into DFU, the serial-triggered reboot,
  the button sequence, the ST-Link alternative.
- `sdcard/`: what goes on the display's SD card.
- `docs/2026-09-01-findings.md` and `docs/MIGRATION-PLAN.md`: state of the project and
  the record of the move away from the Arduino IDE.

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

From `notes.txt`, the working list:

- Communication between screen and controller sometimes stops; cause unknown.
- Pressure readout on the screen is noisy: the controller sends more updates than the
  screen consumes. Kalman filtering is in place, message rate tuning still to do.
- Boiler PID does not settle cleanly; it often drops into the bang-bang band before
  overshooting.
- Steam pressure declines over a long steam; needs a strategy to hold some pressure.
- Full controller logs on the SD card would help tuning; log format needs a header and
  a way to tie settings to a log (hash of the settings).
- Blooming: the pre-infusion fill time may not be consistent between the first and
  subsequent shots; a "stop pump when water detected" command is a candidate fix.
