# Migration plan: Arduino IDE to a scripted arduino-cli workflow

Companion to `docs/2026-09-01-findings.md` (read that first for the "why").
Work happens on branch `migration` in `~/repos/Gaggiano`.

## How to resume after an interruption

1. `git checkout migration && git status && git log --oneline -15` to see what landed.
2. Find the first unchecked box below. Steps are ordered; each phase ends with a
   checkpoint that must pass before moving on.
3. Anything marked **(hardware)** needs the board plugged in and a human at the machine.
4. Commit at the end of every numbered step with a message starting `migration:`.
   Keep commits small so a half-done phase is easy to bisect or revert.

Conventions: `$REPO` = `~/repos/Gaggiano`. `$A15` = `~/Library/Arduino15`.
`gg` = the repo entry-point script introduced in Phase 2.

## Target layout (end state)

```
Gaggiano/
  gg                         # bash entry point: setup | build | detect | flash | monitor | clean
  tools/
    arduino-cli.yaml         # repo-local arduino-cli config (sketchbook = repo, data = $A15)
    targets.sh               # one FQBN + ports + baud per target, sourced by gg
    usb-detect.py            # VID:PID classification helper (python3, stdlib only)
    convertBmp.py            # moved from gaggiano-v2/
  gaggiano-controller-v1/    # unchanged sketch + sketch.yaml (optional, see 2.5)
  gaggiano-v2/               # unchanged sketch + sketch.yaml (optional)
  libraries/                 # vendored libraries, unchanged
  build/                     # gitignored, exported binaries per target
  docs/
    2026-09-01-findings.md
    MIGRATION-PLAN.md
    BUILD.md                 # setup, build, flash, monitor from a fresh clone
    FLASH-STM32.md           # DFU dance, serial-triggered DFU, relay quirk, ST-Link option
    images/
  sdcard/gaggia/             # what goes on the display's SD card
  vendor/sunton-esp32-8048s043/   # spec, schematic, user manual only
  archive/gaggiano-v1/
  .github/workflows/build.yml     # compile both targets on push
```

---

## Phase 0: Preparation (no hardware)

- [x] 0.1 Write `docs/2026-09-01-findings.md` and this plan.
- [x] 0.2 Add `build/` and `.arduino-cli/` (if used) to `.gitignore`. Commit.
- [x] 0.3 Create `tools/arduino-cli.yaml`:
      ```yaml
      board_manager:
        additional_urls:
          - https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
      directories:
        data: /Users/philippepascal/Library/Arduino15   # reuse installed cores
        user: /Users/philippepascal/repos/Gaggiano       # so libraries/ is the sketchbook libs
        downloads: /Users/philippepascal/Library/Arduino15/staging
      sketch:
        always_export_binaries: true
      ```
      Note: `directories.user` must be absolute; `gg setup` should regenerate this file
      from the repo path so it works on another machine (template + sed, or pass
      `--config-dir`). Decide during 2.1; for Phase 1 hard-coding is fine.
      Verify with `arduino-cli --config-file tools/arduino-cli.yaml core list` showing
      `STMicroelectronics:stm32 2.9.0` and `esp32:esp32 2.0.17`.
- [x] 0.4 MOOT (see notes log, isolation). Optional, ask first: point the IDE's `~/.arduinoIDE/arduino-cli.yaml`
      `directories.user` at `~/repos/Gaggiano` so the IDE keeps working as a fallback.

**Checkpoint 0:** `arduino-cli --config-file tools/arduino-cli.yaml lib list` shows the
vendored libraries (lvgl 8.3.3, GFX Library for Arduino 1.2.8, ADS1X15 0.5.2, ...).

---

## Phase 1: Reproduce both builds from the command line (no hardware)

- [x] 1.1 Compile the controller:
      ```
      arduino-cli --config-file tools/arduino-cli.yaml compile \
        --fqbn "STMicroelectronics:stm32:GenF4:pnum=GENERIC_F411CEUX,xserial=generic,usb=CDCgen,xusb=FS,opt=osstd,dbg=none,rtlib=nanofp,upload_method=dfuMethod" \
        --build-property build.warn_data_percentage=75 \
        --output-dir build/controller --warnings default \
        gaggiano-controller-v1
      ```
      Expect `build/controller/gaggiano-controller-v1.ino.bin` around 67 KB and a flash
      usage line similar to the IDE's. Record the size in this file.
- [x] 1.2 Compile the display:
      ```
      arduino-cli --config-file tools/arduino-cli.yaml compile \
        --fqbn "esp32:esp32:esp32s3:UploadSpeed=460800,USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=default,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,EraseFlash=none,JTAGAdapter=default" \
        --build-property build.warn_data_percentage=75 \
        --output-dir build/screen --warnings default \
        gaggiano-v2
      ```
      Expect `.bin` (about 1.05 MB), `.bootloader.bin`, `.partitions.bin`. Record sizes.
      If the build picks up the venv or vendor folders as sources, that is the signal to
      pull Phase 6.1 forward (delete the venv) before continuing.
- [x] 1.3 Compare against the archived IDE builds in `*/archiveArdIDEFiles/` (size, and
      `arm-none-eabi-size` / `xtensa-esp32s3-elf-size` on the `.elf`). Differences must be
      explainable by the source edits since 2025-03-28, not by different flags. Diff the
      compiler command line against the archived `compile_commands.json` first entry if in doubt.

**Checkpoint 1:** both targets compile with zero new warnings and produce binaries. Sizes
recorded here:

| Target | .bin bytes (CLI) | .bin bytes (IDE 2025-03-28) |
|---|---|---|
| controller | 67,492 (text 66,392 vs 66,048) | 67,140 |
| screen | 678,208 (text 466,037 vs 466,665; data 212,056 vs 581,904) | 1,048,672 |

Verified 2026-09-01: the compiler flag sets for the sketch translation unit are identical
between the archived IDE build and the CLI build for both targets (207 and 30 include
paths respectively, same defines). The 370 KB drop in the screen binary is const data:
the March 2025 UI embedded six status icons (`boiler`, `brew`, `steam` and their `Red`
variants) that the current UI no longer references. Their `.c` files are still in the
sketch and are removed in Phase 5.2.

---

## Phase 2: Repo-owned build script and docs (no hardware)

- [x] 2.1 `tools/targets.sh`: one block per target with `FQBN`, `SKETCH_DIR`, `OUT_DIR`,
      `MONITOR_BAUD` (controller 9600, screen 115200), `USB_RUN_VIDPID`, `USB_DFU_VIDPID`
      (controller only: run `0483:5740`, DFU `0483:df11`; screen `1a86:7523`).
- [x] 2.2 `gg` script, bash, `set -euo pipefail`, subcommands:
      - `setup`: check/install Homebrew `arduino-cli`; write `tools/arduino-cli.yaml` from
        the repo path; `core update-index`; `core install STMicroelectronics:stm32@2.9.0`
        and `esp32:esp32@2.0.17` if missing; check STM32CubeProgrammer CLI exists at the
        known path or on `PATH`; print a summary table.
      - `build <controller|screen|all>`: the Phase 1 commands, output to `build/<target>/`.
      - `clean`: remove `build/` and the arduino-cli sketch cache for these sketches.
      - `monitor <target> [port]`: `arduino-cli monitor -p <port> -c baudrate=<baud>`,
        port auto-picked via `detect`.
      - `detect`, `flash`: Phase 3.
      - `--help` on every subcommand; errors are one clear line, not a stack of shell noise.
- [x] 2.3 `docs/BUILD.md`: fresh-clone setup, the three commands (`setup`, `build`,
      `flash`), where binaries land, how to change an FQBN option, how to add a library.
- [x] 2.4 GitHub Actions `.github/workflows/build.yml`: ubuntu runner, install
      arduino-cli, install the two pinned cores, `./gg build all`. Upload binaries as
      artifacts. (Compile only; no upload step.)
- [x] 2.5 SKIPPED. `sketch.yaml` build profiles in each sketch folder pinning the core
      version, so `arduino-cli compile --profile default` works without `gg`. Only worth it
      were skipped on 2026-09-01: profile builds install the platforms again into an
      isolated directory (another ~1 GB download for the esp32 core) and `--dump-profile`
      lists libraries by Library Manager name, which would bypass the vendored copies.
      `gg` already pins cores and options; revisit only if profiles become useful for CI.

**Checkpoint 2:** on a fresh clone in a temp dir, `./gg setup && ./gg build all` succeeds
and produces the same binaries as Phase 1. CI is green.

Result 2026-09-01: passed locally. Controller binary byte-identical from the fresh clone.
Screen binary differs by 3.6 KB only because LVGL logging embeds `__FILE__` absolute
paths (46 path strings, longer in the temp clone). CI not yet observed: the branch has
not been pushed. Check the Actions tab after the first push.

---

## Phase 3: Detection and flashing helpers **(hardware)**

- [x] 3.1 (code written, tested only against a USB hub) `tools/usb-detect.py`: runs `system_profiler SPUSBDataType -json` and
      `arduino-cli board list --format json`, joins on VID:PID, prints one line per known
      device: `controller RUN  /dev/cu.usbmodemXXXX`, `controller DFU  (no port)`,
      `screen /dev/cu.usbserial-XXXX`, plus unknown devices. Exit code 0 if at least one
      known device. Also used by `gg detect`. Fallback for DFU visibility: `dfu-util -l`.
- [x] 3.2 (hardware-tested 2026-09-01: first attempt hit the macOS CH340 'Device not configured' error, retry succeeded at 460800; retry/fallback added to `gg`) `gg flash screen`: `gg build screen`, pick the CH340 port (error with the
      list of candidates if there are several, `--port` override), then run esptool with
      the exact argument list from the findings doc (bootloader 0x0, partitions 0x8000,
      `boot_app0.bin` 0xe000 from `$A15/packages/esp32/hardware/esp32/2.0.17/tools/partitions/`,
      app 0x10000). Prefer `arduino-cli upload` if it reproduces the same command; else
      call esptool directly. Test on the board.
- [x] 3.3 (hardware-tested 2026-09-01 with the button sequence, then buttonless) `gg flash controller` via DFU, in this order:
      1. `gg build controller`.
      2. If a controller in RUN mode is present and firmware supports it (Phase 4), send
         the reboot-to-DFU command on its CDC port.
      3. Poll for `0483:df11` every 0.5 s for up to 30 s. While waiting, print once:
         "Hold BOOT0, tap NRST, release BOOT0" and the relay note from the findings doc.
      4. Flash: `STM32_Programmer_CLI --connect port=usb1 VID=0x0483 PID=0xdf11 --quietMode --download build/controller/<name>.bin 0x8000000 --start 0x8000000`.
         Retry up to 3 times on connect failure (macOS sometimes needs a second attempt).
      5. Wait up to 10 s for `0483:5740` to reappear and report "controller is back".
      Options: `--no-build`, `--timeout N`, `--erase`.
- [x] 3.4 (code written, untested: no probe) `gg flash controller --via stlink` (optional path): `st-flash --reset write
      build/controller/<name>.bin 0x8000000`. Only wire this when an ST-Link is actually
      connected to SWD; document it in `docs/FLASH-STM32.md` as the no-buttons alternative.
- [x] 3.5 `docs/FLASH-STM32.md`: USB identity table, the button sequence with a photo or
      pin diagram (`Black_pill_pinout.png` already in the repo), the relay quirk,
      troubleshooting list (no DFU device: try another cable/port, no hub; DFU seen but
      download fails: retry, or `--erase`; board silent after flash: check `usb=CDCgen`).

**Checkpoint 3:** one successful `gg flash screen` and one `gg flash controller` with the
button sequence, followed by `gg monitor controller` showing "serial works".
PASSED 2026-09-01.

---

## Phase 4: Serial-triggered DFU in the controller firmware **(hardware)**

Goal: `gg flash controller` reboots the board into the ROM bootloader without touching
buttons. The button sequence stays valid forever because the bootloader is in ROM.

- [x] 4.1 Firmware, `gaggiano-controller-v1.ino` plus new `dfu_jump.cpp/.h` (written and compiled 2026-09-01; design changed from RTC backup register to a `.noinit` RAM marker checked by a `constructor(100)`, see notes log):
      - On USB `Serial`, accept a line `DFU` (and `VERSION`, replying with a build
        string so `gg detect` can show firmware version).
      - On `DFU`: set the pump to 0, close the solenoid, boiler PWM to 0, write a magic
        word (e.g. `0xDEADBEEF`) to RTC backup register 0 (`HAL_RTCEx_BKUPWrite`, after
        `HAL_PWR_EnableBkUpAccess()`), flush serial, `NVIC_SystemReset()`.
      - Early boot hook `initVariant()` (first call in `main()`, before `setup()`; USB
        and clocks are already up, see notes log): if the magic is present, clear it,
        detach USB, disable IRQs and SysTick, `HAL_RCC_DeInit()`, `HAL_DeInit()`, set MSP
        from `*(uint32_t*)0x1FFF0000`, call `*(uint32_t*)0x1FFF0004`.
        Keep this code minimal and behind a single `#define ENABLE_SERIAL_DFU 1`.
- [x] 4.2 (2026-09-01: bootloader appears in under 1 s after `DFU`; board returns to RUN after flash; plain resets do not re-enter DFU) Bench test with nothing but USB connected: `echo DFU > /dev/cu.usbmodemXXXX`
      (or via `gg`) must make `0483:df11` appear within 2 s. Then flash, then confirm the
      board comes back in RUN mode and the magic word is cleared (a plain reset must not
      re-enter DFU).
- [x] 4.3 Hook into `gg flash controller` step 2 (Phase 3.3). Keep the button prompt as
      the fallback when no RUN-mode device is present or the command times out.
- [x] 4.4 Safety review (done on the code: `DFU` is parsed only on USB `Serial`; `allOutputsOff()` zeroes pump, valve, boiler PWM and setpoints before the reset) of the firmware change: what happens if `DFU` arrives mid-brew
      (outputs are zeroed first), and confirm the screen cannot send it (it is only parsed
      on USB `Serial`, not on `screenSerial`).

**Checkpoint 4:** three consecutive `gg flash controller` runs with no button presses.
PASSED 2026-09-01. First attempt failed because `gg` wrote the `DFU` line through a
non-blocking open and closed the port before the CDC transfer happened; replaced by
`tools/serial-cmd.py`, which configures the port and keeps it open for a second.

---

## Phase 5: Repository cleanup (no hardware)

Do this only after Checkpoint 3, so every removal can be checked with `gg build all`.
One commit per bullet.

- [x] 5.1 Delete the Python venv in `gaggiano-v2/` (`bin/`, `lib/`, `include/`,
      `pyvenv.cfg`), fix `.gitignore` (`pyenv.cfg` typo), move `convertBmp.py` to `tools/`
      with a one-line usage note (needs Pillow: `python3 -m pip install --user pillow`, or
      `uv run --with pillow`).
- [x] 5.2 Unused LVGL image arrays: grep each symbol (`boiler`, `boilerRed`, `brew`,
      `brewRed`, `steam`, `steamRed`, `img_clothes`, `img_demo_widgets_avatar`,
      `img_lvgl_logo`) across `gaggiano-v2/`. Remove the `.c` files that are unreferenced;
      keep the source PNGs in `gaggiano-v2/asset-work/`. Rebuild; binary size should be
      unchanged (the linker was already discarding them).
- [x] 5.3 `archiveArdIDEFiles/`: keep `build.options.json`, `partitions.csv`, `notes`;
      delete `compile_commands.json`, `.elf`, `.map`, `.bin`, `.hex`, `build_opt.h`,
      `file_opts`. The FQBNs are now in `tools/targets.sh` and the findings doc.
- [x] 5.4 SD card content: create `sdcard/gaggia/` with `frank.bmp` and a README listing
      the files the firmware expects. Remove `frank.jpg` and `frank.rgb565` from the sketch
      folder (regenerable). Move `ESP32-8048S043-macsbug.pdf` and screenshots to `docs/`.
- [x] 5.5 Vendor pack: `git mv` the spec, schematic and user manual folders to
      `vendor/sunton-esp32-8048s043/`; `git rm -r` the rest of `4-2.3inch_ESP32-8048S043/`
      (demos with their own lvgl copy, Windows flash tool, font tools). Add a `README.md`
      there with the vendor download link. No history rewrite.
- [x] 5.6 Scratch sketches `SD_Test/`, `play_lcd/`, `p11_CrystalBall_scroll/`: delete.
      `gaggiano-v1/` to `archive/`. Root images to `docs/images/`.
- [x] 5.7 `git rm --cached` every tracked `.DS_Store`.
- [x] 5.8 Root `README.md`: short project description, hardware list, link to
      `docs/BUILD.md` and `docs/FLASH-STM32.md`, and the current `notes.txt` todo list
      folded in as "Known issues / ideas".

**Checkpoint 5:** `gg build all` still produces identical binaries to Checkpoint 2
(except 5.2 which should be identical too); `git ls-files | wc -l` drops from ~4100 to
roughly 2000 (libraries dominate).
PASSED 2026-09-01: controller 68,080 bytes (same as after Phase 4), screen 678,208 bytes.
Tracked files 4091 -> 1898. Note on the screen binary: two builds of identical source
differ only in the 64 bytes at offsets 177-241, the ESP-IDF app descriptor's ELF SHA-256.
Compare screen binaries with that region masked.

---

## Phase 6: Wrap up

- [x] 6.1 Update this plan's checkboxes and the findings doc with anything learned.
- [ ] 6.2 Open a PR `migration` to `main` (or merge locally) once Checkpoints 2, 3 and 5
      pass. Tag the last IDE-era commit on `main` as `arduino-ide-last` for reference.
- [ ] 6.3 Later, separate effort (not this branch): core upgrades (esp32 3.x, lvgl 9,
      Arduino_GFX current), 16 MB partition table, unit tests for the protocol parser.

## Open questions / notes log

- 2026-09-01 (later): decision to isolate from the Arduino IDE completely. `gg setup`
  now installs cores and toolchains into `.arduino-data/` in the repo (git-ignored)
  instead of sharing `~/Library/Arduino15`. Phase 0.4 (repointing the IDE config) is
  therefore moot. CI caches `.arduino-data/` keyed on `tools/targets.sh`.
  Verification: cores and tool versions installed into `.arduino-data/` are identical to
  the IDE's copies with one exception. The current STM32duino index only offers the
  Intel (x86_64) macOS build of xpack gcc 13.2.1-1.1; the IDE had downloaded the ARM64
  build from an earlier index (its tarball is still in `~/Library/Arduino15/staging`).
  Same compiler and newlib versions built on another host: the controller binary keeps
  the same size and identical symbol sizes but a slightly different layout (1366 bytes
  differ). The screen binary differs only by the longer core source path embedded by
  `__FILE__` in `esp32-hal-uart.c` (+16 bytes) and the resulting address shifts. Both
  builds are deterministic. Both boards were flashed from the isolated build on
  2026-09-01 (controller answered `VERSION`, screen booted). Decision: use what the index provides (reproducible on any
  machine) rather than copying the ARM64 toolchain from the IDE directory.

- 2026-09-01: Phase 4 implementation detail. The marker is two `.noinit` words
  (`dfu_jump.cpp`), set by `dfu_request_reboot()` before `NVIC_SystemReset()`. A
  `constructor(100)` (the core's `premain()` is 101) checks it before any HAL/USB init,
  clears it, disables SysTick and NVIC, remaps system memory to 0, sets MSP and jumps to
  `0x1FFF0004`. Verified in the ELF: `.init_array` lists `dfu_check_marker` first, then
  `premain`; `.noinit` at 0x200018f8 is outside `.bss`. `-Wprio-ctor-dtor` is silenced
  locally. Not yet run on hardware (4.2).
- 2026-09-01: `arduino-cli upload` is used for the actual programming step of both
  targets (it reproduces the IDE's exact esptool / stm32CubeProg.sh invocations); `gg`
  adds detection, the DFU wait loop and retries around it.
- 2026-09-01: `system_profiler SPUSBDataType -json` returns an empty list on this macOS,
  so `usb-detect.py` parses `ioreg -p IOUSB -l` instead.

- 2026-09-01: `sketch.yaml` profiles with local library directories: supported by
  arduino-cli 1.3.1 (`--dump-profile` emits `- dir: <path>` entries), so 2.5 is viable.
- 2026-09-01: `initVariant()` confirmed in the 2.9.0 core `main.cpp`: it is the first
  call in `main()`, before `setup()`. Clocks, HAL and (with `usb=CDCgen`) the USB device
  are already initialised by `premain()` at that point, so the jump code in 4.1 must
  detach USB (`USBDevice.detach()` or `HAL_PCD_DeInit`) and de-init RCC/HAL before
  jumping. Alternative if that proves fragile: a `__attribute__((constructor(101)))`
  function, which runs before `premain()`.
