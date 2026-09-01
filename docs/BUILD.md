# Building and flashing Gaggiano

Everything goes through `./gg`, a small bash script around `arduino-cli`. The Arduino
IDE is no longer needed. Board options, pinned core versions and USB identities live in
one place: `tools/targets.sh`.

## One-time setup (macOS)

```
brew install arduino-cli        # ./gg setup does this for you if brew is present
./gg setup
```

`setup` writes `tools/arduino-cli.yaml` (git-ignored, absolute paths), points the
sketchbook at this repo so the vendored `libraries/` folder is used, updates the package
indexes and installs the two pinned cores if they are missing:

| Target | Core | Version |
|---|---|---|
| controller | STMicroelectronics:stm32 | 2.9.0 |
| screen | esp32:esp32 | 2.0.17 |

Cores are installed in `~/Library/Arduino15`, shared with the Arduino IDE if it is still
installed. If the IDE upgrades a core, `./gg setup` puts the pinned version back.

For flashing the controller you also need STM32CubeProgrammer (free, from ST). `setup`
checks for it at the standard install path.

## Commands

```
./gg build all              # both targets, binaries in build/<target>/
./gg build controller       # or screen; add --clean to drop the compile cache
./gg detect                 # what is plugged in, and on which port
./gg flash screen           # build + esptool over the CH340 port
./gg flash controller       # build + DFU (see docs/FLASH-STM32.md)
./gg monitor controller     # serial monitor at 9600 (screen: 115200)
./gg clean
```

`flash` accepts `--no-build`, `--port /dev/cu.xxx`, `--timeout 120` and
`--via stlink` (controller only, needs an ST-Link on the SWD header).

## Where things are

| Path | Purpose |
|---|---|
| `gaggiano-controller-v1/` | STM32 sketch |
| `gaggiano-v2/` | ESP32-S3 display sketch |
| `libraries/` | vendored Arduino libraries, including the project `lv_conf.h` for LVGL |
| `tools/targets.sh` | FQBN, core version, baud rate and USB ids per target |
| `tools/usb-detect.py` | USB enumeration helper used by `detect` and `flash` |
| `build/` | build output, git-ignored |
| `.github/workflows/build.yml` | CI: compiles both targets on every push |

## Changing a board option

Edit the `T_FQBN` line for the target in `tools/targets.sh`. Valid option names and
values are in the core's `boards.txt`, for example
`~/Library/Arduino15/packages/esp32/hardware/esp32/2.0.17/boards.txt`. The full menu is
also printed by `arduino-cli --config-file tools/arduino-cli.yaml board details --fqbn <fqbn>`.

## Adding a library

Drop it in `libraries/<Name>/` (the Library Manager layout: `library.properties` plus
`src/`). No registration needed; the sketchbook is this repo. Prefer copying a released
version and note the version in the commit message.

## Reproducibility notes

The CLI build was checked against the last Arduino IDE build on 2026-09-01: identical
compiler flags and include paths for both targets. Details in
`docs/2026-09-01-findings.md` and `docs/MIGRATION-PLAN.md`.
