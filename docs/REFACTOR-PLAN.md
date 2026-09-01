# Refactoring plan: fixes, structure and protocol (started 2026-09-01)

Branch `refactoring`, on top of `main` after the migration. Companion documents:
`docs/2026-09-01-findings.md` (project state), `docs/MIGRATION-PLAN.md` (tooling),
`docs/BUILD.md` and `docs/FLASH-STM32.md` (how to build and flash).

## Scope

In: bug fixes, memory safety, code structure, a robust screen/controller protocol,
host-side tests for the pure logic, bench verification of every step.
Out: new features, UI redesign, core or library upgrades (plan 6.3), anything that
changes what the machine does beyond the documented fixes below.

Rule for every step: it compiles with `./gg build all`, it is flashed with `./gg flash`,
and the bench checklist (R0.3) passes before the next step starts. One commit per
numbered step, message prefixed `refactor:`.

## How to resume after an interruption

1. `git checkout refactoring && git status && git log --oneline -15`.
2. Find the first unchecked box. Steps are ordered inside a phase; phases are ordered.
3. Steps marked **(bench)** need both boards on USB. Nothing here needs the boards in
   the machine until the final soak in R3.
4. Record surprises in the notes log at the end of this file.

## Decisions (answered 2026-09-01)

- D1. Link-loss behaviour on the controller: after N seconds without a valid command,
  pump off and valve closed for sure. Boiler: keep the last temperature setpoint
  (machine stays hot, proposed) or drop to 0? Proposed N = 3 s.
  **Decided: keep the boiler setpoint. N = 3 s.**
- D2. Protocol stays text with a checksum (proposed), not binary. **Decided: yes.**
- D3. Profile names capped at 24 characters (SD filename budget). Longer names are
  truncated on rename. **Default accepted.**
- D4. Enable the STM32 independent watchdog (resets the board if the loop hangs; a
  reset means pump off, valve closed, boiler PWM 0 until the screen re-sends). Proposed yes.
  **Decided: yes, try it.**
- D5. Host tests: plain C++ with a tiny assert helper and a Makefile, run by `./gg test`.
  No framework dependency. Proposed yes. **Default accepted.**
- D6. `double` to `float` on the controller where values only feed float APIs. The
  F411 has a single-precision FPU; doubles are emulated. Proposed: do it only in the
  new code paths (protocol, sensors); leave AutoPID's doubles alone. **Default accepted.**

## Bugs found by reading the code (fixed in the phases below)

| # | Where | What | Effect |
|---|---|---|---|
| B1 | both sides, `mySubString` (three copies) | `malloc` per field, never freed | screen leaks ~4 blocks per status line (5/s); controller leaks per command. Likely the "comms sometimes stop" symptom |
| B2 | controller `readTemperature` | `newReading > 1 \|\| newReading < 200` always true | MAX6675 fault/zero readings pass into the PID |
| B3 | controller `loop` | `LOOP_PERIOD - (millis() - loopStart)` is unsigned; a loop over 10 ms yields a ~49-day `delay()` | firmware freezes with outputs held at their last value |
| B4 | controller `parseMessage` | unknown or truncated message falls through to `operating_mode = BREW` | mode switch on line noise, e.g. during steam |
| B5 | both sides | no checksum, no field-count check | a flipped digit changes a setpoint silently |
| B6 | controller | UART RX buffer 64 bytes, read every 200 ms; longest command 58 bytes | two commands within 200 ms truncate the second |
| B7 | screen | commands sent once, never refreshed; controller never times out | a lost "stop" keeps the pump running; a dead link freezes the machine state |
| B8 | screen `readMessage` | `readStringUntil` blocks up to 50 ms in the UI loop; one line per pass | UI stutter, stale or skipped status lines |
| B9 | screen `gaggia_config.cpp` | `malloc(30)` + `strcat` of profile names; `sprintf` of notes into 500 bytes; `new char[500]` never freed | stack/heap overflow with long names or notes; leak per profile load |
| B10 | screen `setupAndReadConfigFile` | reads `values[0..18]` without checking how many rows the CSV had | garbage settings from an old or truncated profile file |
| B11 | screen `state.notes`, `state.profile_name` | `char*` pointing at leaked heap or string literals | undefined lifetime, leak per load |
| B12 | controller `parseMessage` | 500-byte line buffer on the stack plus `String` from `readStringUntil` | heap churn, stack pressure; no bound on line length |

## Target layout (end state)

```
libraries/GaggiaProtocol/          shared by both sketches and the host tests
  library.properties
  src/gaggia_protocol.h            message types, field lists, limits, version
  src/gaggia_protocol.cpp          encode / decode / checksum, no heap, no Arduino deps
  src/line_reader.h                non-blocking line assembler over any byte source
gaggiano-controller-v1/
  gaggiano-controller-v1.ino       setup() / loop() only
  config.h                         pins, periods, limits
  sensors.{h,cpp}                  MAX6675 + ADS1115 + Kalman
  outputs.{h,cpp}                  boiler PWM, pump (PSM), valve, allOutputsOff()
  control.{h,cpp}                  mode logic (brew / steam / clean), PID glue
  link.{h,cpp}                     screen UART: STAT out, CMD/TUNE in, link timeout
  console.{h,cpp}                  USB serial: VERSION, DFU, STATUS, LOG
  dfu_jump.{h,cpp}                 unchanged
  PSM.{h,cpp}                      unchanged
  build_opt.h                      -DSERIAL_RX_BUFFER_SIZE=256
gaggiano-v2/
  gaggiano-v2.ino                  setup() / loop() only
  display_glue.{h,cpp}             Arduino_GFX panel, LVGL flush, touch (unchanged behaviour)
  sequencer.{h,cpp}                brew/bloom/auto phase machine, pure logic, time injected
  link.{h,cpp}                     controller UART: CMD/TUNE out with heartbeat, STAT in
  storage.{h,cpp}                  SD: profiles, settings, logs (was gaggia_config.cpp)
  gaggia_state.h                   fixed-size strings instead of char*
  lv_buildUI.{h,c}                 UI only; no parsing helpers
tests/
  Makefile                         host build with the system compiler
  test_protocol.cpp, test_sequencer.cpp, test_line_reader.cpp
docs/PROTOCOL.md                   the wire format, one page
```

---

## Phase R0: baseline and safety net (no bench)

- [x] R0.1 Starting commit: `main` at 2f58621 (Migration #1). Save the current
      binaries as `build-baseline/` locally (git-ignored) for size comparison.
- [x] R0.2 `tests/` with a Makefile that compiles listed `.cpp` files with the host
      `c++`, plus `./gg test`. A first trivial test proves the harness. CI runs it.
- [x] R0.3 `docs/BENCH-CHECKLIST.md`: the manual verification run after every flash.
      Draft:
      1. `./gg detect` shows both boards. `./gg monitor controller`, `VERSION` answers.
      2. Screen boots to the main tab; temperature and pressure update about 5 times/s.
      3. Heat on: controller log shows the setpoint; boiler output rises. Heat off.
      4. Brew on for 5 s: valve opens, pump output non-zero, brew timer counts. Brew off:
         valve closes, pump 0.
      5. Steam on/off, clean on/off: mode echoed by the controller.
      6. Settings tab: change a value, Set, reboot the screen, value persists.
      7. Profile tab: duplicate, rename, select, delete. Names survive a reboot.
      8. Leave both boards running 10 minutes; free heap (screen) and loop counter
         (controller) still reported and stable.
      Items 1-2 after every flash; the full list at each checkpoint.
- [ ] R0.4 Controller console additions needed for the checklist: `STATUS` (mode,
      setpoints, outputs, loop counter, max loop time, link state) and `LOG ON|OFF`
      (silence the per-message debug spam). Screen: print free heap every 10 s to USB.

**Checkpoint R0:** `./gg test` passes, checklist items 1-2 pass with the current
firmware, baseline sizes recorded: controller 68,080 bytes, screen 678,224 bytes.

---

## Phase R1: controller fixes and structure (no protocol change) **(bench)**

The wire format stays exactly as today so the screen keeps working unmodified.

- [ ] R1.1 Fix B3: compute elapsed time, `delay` only when elapsed is below the period;
      track the maximum loop time for `STATUS`.
- [ ] R1.2 Fix B2 (`&&`), and treat a MAX6675 open-thermocouple reading (NaN or 0) as a
      fault: keep the last good value, count faults, expose in `STATUS`.
- [ ] R1.3 Fix B4/B12/B1 on the controller: replace `parseMessage` with a non-blocking
      line assembler (fixed 128-byte buffer, characters pulled every loop pass) and an
      in-place `strtod` field parser. Unknown type or wrong field count: ignore, count.
      Delete `mySubString`, `myIndexOF`, the dead `updatePump`.
- [ ] R1.4 Fix B6: `build_opt.h` with `-DSERIAL_RX_BUFFER_SIZE=256`; verify the flag is
      honoured in the verbose build output.
- [ ] R1.5 Split the sketch into the files listed above. Pure move, no logic change;
      confirm the `.bin` size is within a few hundred bytes of R1.4.
- [ ] R1.6 D4: enable the independent watchdog (STM32duino `IWatchdog`, 4 s), reloaded
      once per loop. Verify: `HANG` console command spins forever; the board resets
      within 4 s and comes back with outputs off.
- [ ] R1.7 D6 in the touched code only.

**Checkpoint R1:** full bench checklist with the unmodified screen firmware. Ten
minutes of heat + a 30 s brew with the console showing loop max time under 10 ms.

---

## Phase R2: shared protocol v2 **(bench)**

Both sides change together; both boards are flashed together.

- [ ] R2.1 `docs/PROTOCOL.md`. Proposed format (D2), NMEA-style:

      ```
      $TYPE,field,field,...*HH\n
      ```
      `HH` is the XOR of every byte between `$` and `*`, two upper-case hex digits.
      Fields are decimal with at most 2 decimals; a receiver rejects a line whose type
      is unknown, whose checksum fails, or whose field count is wrong. Line length is
      capped at 120 bytes.

      | Type | Direction | Fields | When |
      |---|---|---|---|
      | `HELLO` | both | protocol version, firmware version string | on boot, and as reply to `HELLO` |
      | `STAT` | controller to screen | mode, temp, pressure, valve, boilerOut, pumpOut, tempSet, pressSet, pumpPct, linkOk, faults, counter | every 200 ms |
      | `CMD` | screen to controller | mode (0 off, 1 brew, 2 steam, 3 clean), tempSet, pressSet, pumpPct | on change and every 1 s (heartbeat) |
      | `TUNE` | screen to controller | bbRange, pidCycle, kp, ki, kd, pumpStepUp, pumpKp, pumpKi, pumpKd | on change, and after every controller `HELLO` |

      Controller rules: no valid `CMD` for N s (D1) sets mode 0, pump 0, valve closed,
      `linkOk=0` in `STAT`. Screen rules: if `STAT` echoes a mode or setpoint different
      from what it last sent for more than 1 s, re-send `CMD`. `TUNE` is re-sent whenever
      the controller says `HELLO` (it rebooted). The SD log keeps the raw `STAT` lines;
      the header row changes accordingly.
- [ ] R2.2 `libraries/GaggiaProtocol`: encoder, decoder, checksum, `line_reader.h`.
      No heap, no `String`, no Arduino types (a `putc`-style callback for output). Host
      tests: round trip of every message, checksum failure, truncated line, garbage
      before `$`, field count mismatch, oversize line, two lines in one read.
- [ ] R2.3 Controller `link.cpp` on the library: `STAT` every 200 ms, `HELLO` at boot,
      `CMD`/`TUNE` handling, link timeout. Mode changes only from a valid `CMD`.
- [ ] R2.4 Screen `link.cpp` on the library: heartbeat, echo check, `TUNE` after `HELLO`,
      RX buffer 512 (`setRxBufferSize` before `begin`), all pending lines drained each
      pass, newest `STAT` wins.
- [ ] R2.5 Flash both. Bench: full checklist, then pull the UART wire mid-brew: pump
      stops within N s, `STAT` shows `linkOk=0` on the console; reconnect: screen
      re-sends within 1 s, brew resumes only if still selected on the screen. Reset the
      controller with NRST while heating: screen re-sends `TUNE` and `CMD` unprompted.

**Checkpoint R2:** the above, plus `./gg test` green.

---

## Phase R3: screen fixes and structure **(bench)**

- [ ] R3.1 Fix B9/B10/B11: `gaggia_state.h` gets `char profile_name[32]` and
      `char notes[128]`; all filename building through one `snprintf` helper with the D3
      cap; the CSV loader checks the row count and falls back to defaults per missing
      field; every `malloc`/`new` in `storage.cpp` and `lv_buildUI.c` removed or paired
      with a free (target: zero heap allocation after boot outside LVGL and the SD driver).
- [ ] R3.2 Fix B8 leftovers: the `i < 20` loop hack becomes time based (`STAT` consumption
      and UI refresh at 5 Hz, `lv_timer_handler` every 5 ms); no blocking reads remain.
- [ ] R3.3 `sequencer.cpp`: the bloom/auto/brew phase machine extracted from
      `sendCommand` into a pure function of (state, now) that returns the desired `CMD`.
      Host tests for the phase transitions and timers.
- [ ] R3.4 `display_glue.cpp`: panel, LVGL driver, touch and splash moved out of the
      `.ino` unchanged.
- [ ] R3.5 Logging: `logController` keeps the file open between lines and flushes once a
      second; `deleteLogsFile` writes the new header. Free-heap line to USB every 10 s.
- [ ] R3.6 `lv_buildUI.c`: remove the parsing helpers, use the fixed-size strings, and
      fold the ten copy-pasted settings-field blocks into one helper. Pixel-identical
      screens; compare by eye against `docs/images/screen-ui-2025-03-21.png`.
- [ ] R3.7 Debug output behind one log-level switch on both sides.

**Checkpoint R3:** full bench checklist, then a 1-hour soak with both boards on USB:
free heap flat on the screen, loop max time stable on the controller, no missed
`STAT` lines in the SD log (counter field contiguous).

---

## Phase R4: wrap-up

- [ ] R4.1 README: protocol summary and link to `docs/PROTOCOL.md`; findings doc gets a
      "resolved" column for B1-B12; this plan's boxes ticked.
- [ ] R4.2 One test in the machine: heat, one shot, one steam. Compare against how it
      felt before the refactor; no tuning changes are expected.
- [ ] R4.3 PR `refactoring` to `main`.

## Deferred (not this branch)

- Core and library upgrades (migration plan 6.3), 16 MB partition table.
- Items from `notes.txt`: pressure readout smoothing strategy, PID retune, steam
  pressure hold, "stop pump when water detected" prime command, settings hash in logs.
- `double` to `float` inside AutoPID and the PID tuning that would need re-validation.

## Notes log

- 2026-09-01: plan written from a full read of both sketches, `gaggia_config.cpp`,
  `PSM.cpp` and the UI event handlers. The three `mySubString` copies (controller,
  `gaggia_utils.cpp`, `lv_buildUI.c`) all leak. B3 (unsigned delay underflow) was
  found while planning and is possibly the more direct cause of "comms stop".
