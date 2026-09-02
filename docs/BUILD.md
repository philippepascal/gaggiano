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
sketchbook at this repo so the vendored `libraries/` folder is used, and installs the two
pinned cores with their toolchains into `.arduino-data/` inside the repo (git-ignored,
a few GB, one-time download):

| Target | Core | Version |
|---|---|---|
| controller | STMicroelectronics:stm32 | 2.9.0 |
| screen | esp32:esp32 | 3.3.11 |

Nothing outside the repo is read at build time except the compilers' own caches in
`~/Library/Caches/arduino`. The Arduino IDE and its `~/Library/Arduino15` directory are
not used at all; they can be removed. `./gg clean --all` deletes `.arduino-data/` and
the next `./gg setup` downloads it again.

Flashing the controller uses `dfu-util` from Homebrew; `setup` installs it. ST's
STM32CubeProgrammer is optional (`--via cubeprog`).

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

## Updating the screen over WiFi

Once the screen is on the network (`docs/WEB.md`):

```
./gg flash screen --ota            # host gaggiano.local
./gg flash screen --ota 192.168.1.42
```

The build is sent as an HTTP upload to the screen's own web server (`POST /update`), so
it works whenever the Mac can reach the screen, including across an IoT network. The
panel goes dark while the flash is written (redraws would come out garbled) and comes
back with the restart; the progress is printed on the USB console. The web page has the same upload under
"Update firmware", usable from a phone. The password is `gaggiano` unless
`tools/ota-password` (git-ignored) says otherwise; the screen's own password is set on
its WiFi view ("Firmware update" password) and defaults to `gaggiano`. The USB path keeps working as before.

## Partition scheme of the screen

The screen uses the core's `app3M_fat9M_16MB` scheme: two 3 MB app slots (needed for
over-the-air updates) and a 9.9 MB FAT partition. The first flash after changing the
scheme (2026-09-02, core 3.3.11) must erase the whole chip once:

```
./gg flash screen --erase
```

Profiles and logs live on the SD card and are not affected.

## Troubleshooting the screen upload

| Symptom | Meaning |
|---|---|
| `Could not configure port: (6, 'Device not configured')` | macOS CH340 quirk on the first open after plugging in. `gg` retries three times and drops to 115200 on the last try. Plugging directly into the Mac (no hub) helps. |
| `detect` shows the screen as `usbserial-XXXX` although the WCH driver is installed | Apple's built-in CH340 driver claimed the device. Both work; if uploads keep failing, disable one driver. |
| No port at all | the board's CH340 needs a data cable; the S3's own USB connector is not used by this build. |

## Where things are

| Path | Purpose |
|---|---|
| `gaggiano-controller-v1/` | STM32 sketch |
| `gaggiano-v2/` | ESP32-S3 display sketch |
| `libraries/` | vendored Arduino libraries, including the project `lv_conf.h` for LVGL |
| `tools/targets.sh` | FQBN, core version, baud rate and USB ids per target |
| `tools/usb-detect.py` | USB enumeration helper used by `detect` and `flash` |
| `tools/serial-cmd.py` | send one line to a serial port and print the reply; `--list` shows ports, `--help` for usage |
| `tools/convertBmp.py` | PNG/BMP to raw RGB565 (needs Pillow) |
| `sdcard/` | files for the display's SD card |
| `vendor/sunton-esp32-8048s043/` | display board datasheets, schematics, manual |
| `archive/gaggiano-v1/` | first display firmware, kept for reference |
| `build/` | build output, git-ignored |
| `.github/workflows/build.yml` | CI: compiles both targets on every push |

## Changing a board option

Edit the `T_FQBN` line for the target in `tools/targets.sh`. Valid option names and
values are in the core's `boards.txt`, for example
`.arduino-data/packages/esp32/hardware/esp32/3.3.11/boards.txt`. The full menu is
also printed by `arduino-cli --config-file tools/arduino-cli.yaml board details --fqbn <fqbn>`.

## Adding a library

Drop it in `libraries/<Name>/` (the Library Manager layout: `library.properties` plus
`src/`). No registration needed; the sketchbook is this repo. Prefer copying a released
version and note the version in the commit message.

## Reproducibility notes

The CLI build was checked against the last Arduino IDE build on 2026-09-01: identical
compiler flags and include paths for both targets. Details in
`docs/2026-09-01-findings.md` and `docs/MIGRATION-PLAN.md`.
