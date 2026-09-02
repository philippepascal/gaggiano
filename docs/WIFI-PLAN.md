# Plan: core upgrade, WiFi, time, web page, OTA (started 2026-09-02)

Branch `advancedStuff`, on top of `main` after the UI redesign. Companion documents:
`docs/UI-PLAN.md` (UI structure, simulator), `docs/BUILD.md`, `docs/BENCH-CHECKLIST.md`.

## Decisions (2026-09-02)

- LVGL stays at 8.3.3. The esp32 core moves to 3.3.11 and Arduino_GFX to its current
  release, which the new core requires. The controller is untouched.
- Features, in priority order: 1. WiFi setup view reached from a button on the Advanced
  view (not from the menu); 2. time via NTP, time zone chosen on the WiFi view; 3. a web
  page with status, a live graph and a log download for spreadsheets, no login; 4. OTA
  firmware update of the screen; 5. optional Bluetooth scale.
- Credentials and time zone live in NVS (the `Preferences` library), not on the SD card.
- The screen is off the bench for now: everything is built and exercised in the simulator
  and the host tests; bench steps are marked **(bench)** and batched for when it is back.

## How to resume after an interruption

1. `git checkout advancedStuff && git status && git log --oneline -15`.
2. Find the first unchecked box. Phases are ordered.
3. One commit per numbered step, message prefixed `wifi:` (or `core:` for W1).

## Gotchas

- Flash: the screen is at 755 KB of a 1.25 MB app slot. WiFi + web server add a few
  hundred KB and OTA needs two slots. W1 switches to the built-in 16 MB partition scheme
  with two 3 MB app slots and a 9 MB FAT data partition (`app3M_fat9M_16MB`). That is a
  full erase of the chip; profiles are on the SD card and survive.
- Core 3.x changes the FQBN option names; `tools/targets.sh` is the only place to edit.
- Arduino_GFX's RGB panel API changed between 1.2.8 and 1.6: `display_glue.cpp` is the
  only file that knows the panel.
- WiFi on the S3 shares internal RAM with everything else. The draw buffer (192 KB) must
  stay in internal RAM; LVGL is already in PSRAM. Check `STATUS` heap after each phase;
  WiFi needs about 40 KB when connected.
- The simulator does not do WiFi: the network module gets a `netStatus()` interface the
  simulator fakes, so the WiFi view and the header clock render on the Mac.
- The web server runs in the loop (the core's synchronous `WebServer`): each request must
  be short. The log download streams the file in chunks; the live page polls a JSON
  endpoint once a second.

## Target layout (end state)

```
gaggiano-v2/
  net.{h,cpp}          WiFi connect/reconnect, NVS credentials, NTP, time zone, status
  webui.{h,cpp}        HTTP server: /, /api/status, /api/history, /logs.csv, mDNS gaggiano.local
  ota.{h,cpp}          ArduinoOTA hooks (5.)
  lv_buildUI.c         WiFi view (button on Advanced), clock in the header
  storage.cpp          log lines with a timestamp once time is known
tools/targets.sh       core 3.3.11 FQBN, 16 MB partition scheme
gg                     flash screen --ota <host>
tests/test_timezones.cpp
docs/WEB.md            what the page shows, the JSON, the CSV columns
```

---

## Phase W1: core upgrade (screen)

- [x] W1.1 `tools/targets.sh`: `ESP32_CORE_VERSION=3.3.11`, FQBN rewritten with the 3.x
      option names, `PartitionScheme=app3M_fat9M_16MB`. `./gg setup` installs the core.
- [x] W1.2 Vendored `libraries/GFX_Library_for_Arduino` replaced by the current release
      (`arduino-cli lib install` into the sketchbook, then committed).
- [x] W1.3 `display_glue.cpp` ported to the new panel classes (`Arduino_ESP32RGBPanel`
      with the timings, `Arduino_RGB_Display`), same pins and timings as today.
- [x] W1.4 Build clean for the board; fix what the new core flags. Record flash and RAM.
      Simulator untouched (LVGL unchanged).
- [ ] W1.5 **(bench)** `./gg flash screen` with a full erase once (`EraseFlash=all` for
      that flash only, documented in BUILD.md), then the bench checklist items 1 to 9.
      `STATUS` heap noted as the new baseline.

**Checkpoint W1:** the redesigned UI runs unchanged on core 3.3.11.

---

## Phase W2: WiFi setup view and connection

- [x] W2.1 `net.{h,cpp}`: credentials and time zone in NVS (`Preferences`, namespace
      `gaggiano`); `netBegin()` connects at boot if credentials exist; reconnect with
      backoff; `netStatus()` returns state, SSID, IP, RSSI; `netScan()` lists networks
      (asynchronous, results polled). `netSetCredentials()` saves and reconnects.
- [x] W2.2 WiFi view (tab 6, not in the menu): reached from a "WiFi" button on the
      Advanced view, with the usual back through the profile name. Contents: network
      list from the scan (tap to pick), password field (editor overlay, full keyboard),
      time zone dropdown, Connect / Forget buttons, a status line (connected as ..., IP,
      signal). Rendered in the simulator with a fake status.
- [x] W2.3 Header: a small WiFi symbol next to the menu when connected (dim when not).
- [x] W2.4 `STATUS` console line gains wifi state, IP, RSSI.
- [ ] W2.5 **(bench)** join the home network from the panel, power cycle, it reconnects
      by itself. Heap before/after connection recorded.

**Checkpoint W2:** the screen is on the network after a power cycle without touching it.

---

## Phase W3: time

- [x] W3.1 Time zone table: 12 common zones as (name, POSIX TZ string) in
      `timezones.{h,cpp}` (pure, host-tested for the format), default
      America/Los_Angeles; the WiFi view dropdown uses it.
- [x] W3.2 `net.cpp`: `configTzTime()` once connected; `netTimeValid()` when the clock is
      set. Header shows the clock (HH:MM) left of the WiFi symbol when valid.
- [x] W3.3 `storage.cpp`: log lines prefixed with the ISO timestamp when time is valid,
      `-` otherwise; header row updated. "Last shot" keeps seconds; the log carries the
      wall clock.
- [ ] W3.4 **(bench)** clock correct on the panel after boot; a log line shows the time.

**Checkpoint W3:** correct local time on the header and in the log.

---

## Phase W4: web page

- [x] W4.1 `webui.{h,cpp}`: `WebServer` on port 80, mDNS `gaggiano.local`. Endpoints:
      `/` (one HTML page, inline CSS/JS, no external assets), `/api/status` (JSON: the
      STATUS fields plus profile and setpoints), `/api/history` (JSON arrays of the
      graph rings), `/logs.csv` (streams the SD log; `Content-Disposition` attachment).
- [x] W4.2 The page: header with profile and state, three big readings, a canvas chart
      fed from `/api/history` at load and `/api/status` every second (same colors as
      the panel), a "Download log" button. Works on a phone. Source kept as a `.html`
      file in `gaggiano-v2/web/` and embedded at build time (a tiny script turns it into
      a C string, run by `./gg build`, committed output).
- [x] W4.3 Log CSV columns documented in `docs/WEB.md`: timestamp, mode, temp, pressure,
      valve, heater, pump, tempSet, pressSet, pumpPct, linkOk, faults, counter. The
      firmware writes that format from W3.3 on (one line per STAT, 5 per second; a
      setting to log at 1 Hz instead if the card fills).
- [ ] W4.4 **(bench)** open `http://gaggiano.local` on the phone during a brew; the graph
      follows the panel; the download opens in a spreadsheet.

**Checkpoint W4:** live page and log download from a phone.

---

## Phase W5: over-the-air update

- [x] W5.1 `ota.{h,cpp}`: `ArduinoOTA` with a password from NVS (default set at build,
      changeable on the WiFi view), hostname `gaggiano`; progress shown on the panel;
      actions stopped and the controller sent an off command before the update.
- [x] W5.2 `./gg flash screen --ota [host]`: `arduino-cli upload` with the network port
      (`gaggiano.local` by default), password from `tools/ota-password` (git-ignored).
- [ ] W5.3 **(bench)** one OTA update from the Mac with no cable, then a power cycle.

**Checkpoint W5:** the screen updates over WiFi; the USB path still works.

---

## Phase W6: Bluetooth scale (optional)

- [ ] W6.1 Survey which scale (Acaia, Bookoo, Felicita, Decent) and its BLE protocol;
      a `scale.{h,cpp}` with `scaleWeight()`, `scaleTare()`, connection state.
- [ ] W6.2 Weight tile on the main screen when a scale is connected; brew-by-weight as a
      sequencer option (stop at target weight minus a drip offset), host-tested.
- [ ] W6.3 **(bench)** with the actual scale.

## Deferred

- LVGL 9 port. Controller OTA through the screen. Home Assistant / MQTT.
- The delete-selects-first-profile quirk (UI plan U4.2).

## Notes log

- 2026-09-02 W4/W5 built, bench pending: `webui.cpp` (WebServer, `/`, `/api/status`,
  `/api/history` streamed, `/logs.csv`, mDNS once connected), the page in
  `gaggiano-v2/web/index.html` embedded by `tools/embed-web.py` from `./gg build`,
  CSV log rows (every line while running, 1/s idle; logging on by default), `ota.cpp`
  (ArduinoOTA, progress notice on the panel, actions stopped first), `./gg flash screen
  --ota [host]` through the core's `espota.py`. Firmware 1,488,659 bytes (47 %), static
  RAM 67 KB.

  Bench list for when the screen is back: W1.5 (`./gg flash screen --erase`, checklist
  1-9, heap baseline), W2.5 (join the network, power cycle), W3.4 (clock, log line),
  W4.4 (page on the phone during a brew, CSV in a spreadsheet), W5.3 (one OTA update).

- 2026-09-02 W2/W3 built and rendered in the simulator (`--scene wifi`, `wifi-off`),
  bench pending: `net.cpp` (WiFi STA with reconnect, credentials and zone in NVS, SNTP
  via `configTzTime`), `timezones.cpp` (12 zones, tested), WiFi view reached from the
  Advanced view, header clock and WiFi symbol, `STATUS` fields, timestamped log lines.
  Firmware 1,386,571 bytes (44 % of the 3 MB slot): the WiFi stack adds ~600 KB, which
  is why the partition change came first. Static RAM 62 KB.

- 2026-09-02: plan written; core 3.3.11 installed into `.arduino-data` (2.0.17 kept
  alongside for a rollback). Arduino_GFX 1.2.8 -> 1.6.7. The screen built on the new
  core at the first attempt: 790,416 bytes (25 % of the 3 MB slot), static RAM 37,856.
  `./gg flash screen --erase` added for the one-time partition change.
