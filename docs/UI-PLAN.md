# UI plan: main screen redesign (started 2026-09-02)

Branch `UIv2`, on top of `main` after the refactor. Companion documents:
`docs/REFACTOR-PLAN.md` (what the code looks like now), `docs/BENCH-CHECKLIST.md`.

## Goals

A main screen that reads at a glance from where you stand at the machine, in a look
that sits next to brushed stainless and a black bezel. Agreed on 2026-09-01/02:

- Header down to 44 px. Profile name on the left (no "profile" label), the notes in
  muted text after it on the same line, a menu icon on the right instead of tabs.
- Readings first: three tiles (boiler temperature, pressure, timer) with a big live
  value and a mini curve of the last 30 s behind it. No setpoint text in the tiles.
- Six buttons in two rows with 12 px gaps that fill the remaining height. Name on one
  line, value on a second, both large.
- One warm accent (amber) for whatever is running, steel blue for steam. Anything
  unavailable while an action runs dims; the layout never jumps.
- Palette: ground `#0F1113`, surface `#171A1D`, steel `#3C434A`, text `#E9ECEF`,
  muted `#8B949E`, amber `#D9A441`, steam `#7FB3D5`.
- Fonts: Montserrat 14 (labels, menu), 20 (button names), 28 (button values),
  48 (tile values). All four are built into lvgl 8.3; 24/26/36 get disabled.

Out of scope on this branch: new behaviour, protocol changes, core upgrades. The
other three views only receive the palette; their layouts are a later pass.

## How to resume after an interruption

1. `git checkout UIv2 && git status && git log --oneline -15`.
2. Find the first unchecked box. Phases are ordered; U1 (the simulator) comes first
   because every later step iterates in it.
3. **(bench)** marks steps that need the screen on USB. Everything else runs on the Mac.
4. One commit per numbered step, message prefixed `ui:`.

## Gotchas to respect throughout

- Rendering: quarter-screen draw buffer, 16-bit color. Flat fills, 1 px borders,
  radii up to 8, no shadows, no gradients (they band).
- Flash: app slot 1,310,720 bytes, currently 679,744 used. The font change is about
  +80 KB. Check the size after U2.
- The UI is C (`lv_buildUI.c`) compiled with the Arduino core; the simulator compiles
  the same file on the Mac, so it must stay free of Arduino types. `millis()` is the
  only Arduino call it makes today.
- All styles live in one file (`theme.c`) so the lvgl 9 port later touches only that
  file and `display_glue.cpp`.
- Every step ends with the UI running in the simulator; bench flashes happen at the
  end of each phase, not each step.

## Target layout (end state)

```
gaggiano-v2/
  theme.{h,c}          palette, fonts, styles by role (tile, button, header, menu)
  history.{h,cpp}      ring buffers of readings for the curves (pure, tested)
  lv_buildUI.c         screens built from theme roles; no colors or sizes inline
sim/
  Makefile             builds lvgl + lv_buildUI.c + theme.c against SDL2 on the Mac
  main.cpp             window, LVGL flush and mouse drivers, fake state driver
  Arduino.h            shim: millis()
  my_logging.cpp       shim: printf
tests/test_history.cpp
```

---

## Phase U1: simulator on the Mac (no bench)

- [x] U1.1 `sim/Makefile`: compiles every `.c` under `libraries/lvgl/src`, plus
      `gaggiano-v2/lv_buildUI.c`, `theme.c`, `history.cpp`, `sequencer.cpp`, and
      `sim/main.cpp`, with `-I libraries` (for `lv_conf.h`), `-I gaggiano-v2`, SDL2
      from `sdl2-config`. Object files under `sim/build/`, git-ignored. `./gg sim`
      builds and runs it.
- [x] U1.2 `sim/main.cpp`: 800x480 SDL window; LVGL display driver whose flush copies
      the draw buffer into an SDL texture; pointer driver from SDL mouse events; a
      fake `GaggiaStateT` and `AdvancedSettingsT`; stub callbacks for the storage
      functions (profiles from a small in-memory list); keys to drive the state:
      `t`/`T` temperature up/down, `p`/`P` pressure, `b` toggles a fake brew, `s`
      steam, `q` quit. The sequencer runs in the loop so bloom/auto behave.
- [x] U1.3 The current UI (before any redesign) runs in the window. Screenshot it
      into `docs/images/ui-before.png` with SDL (key `w` writes a BMP; convert once).

**Checkpoint U1:** `./gg sim` opens the window and the old UI is usable with the mouse.
Status 2026-09-02: headless `--shot` verified (docs/images/ui-before.png); the interactive
window needs a GUI session, to be tried by hand.

---

## Phase U2: theme file and fonts (no bench)

- [x] U2.1 `libraries/lv_conf.h`: enable Montserrat 14, 20, 28, 48; disable 24, 26, 36
      (check nothing else uses them: `grep montserrat gaggiano-v2/`). Default font 14.
- [x] U2.2 `theme.{h,c}`: the palette as `lv_color_t` constants, the four fonts, and one
      `lv_style_t` per role: screen, header, header_title, header_notes, menu_btn,
      tile, tile_value, tile_label, tile_curve, btn, btn_on, btn_on_steam, btn_dis,
      btn_name, btn_value, notes. `theme_init()` builds them; `theme_apply_<role>(obj)`
      helpers. `lv_theme_default_init` stays as the base (dark, with our primary) so
      widgets we do not style explicitly still look consistent.
- [x] U2.3 Apply the theme to the existing screens without changing layouts. Run in the
      simulator. Build for the board and record the flash size.

**Checkpoint U2:** old layout, new palette and fonts, in the simulator; board build
under 800 KB.
PASSED 2026-09-02: docs/images/ui-u2-palette.png; board build 653,312 bytes (was 679,744:
the dropped 24/26/36 fonts weighed more than the new 20/28/48).

---

## Phase U3: main screen (no bench until the end)

- [x] U3.1 `history.{h,cpp}`: `HistoryBuffer` with a fixed capacity (150 points),
      `push(value)`, `count()`, `at(i)`; three buffers (temperature, pressure, timer
      is not curved). Fed from `updateUI` at 5 Hz for pressure and 1 Hz for
      temperature (2-minute window). Host test: wrap-around, ordering, empty state.
- [x] U3.2 Header: 44 px row; profile label (Montserrat 20, text color), notes label
      (14, muted, single line, clipped with an ellipsis, hidden when empty), menu icon
      button on the right (`LV_SYMBOL_LIST`). The tabview keeps the views but its tab
      bar height is 0; the menu is a dropdown with Profiles, Settings, Advanced and
      switches with `lv_tabview_set_act`. On a non-main view the icon shows
      `LV_SYMBOL_LEFT` and returns to Main. Disabling the tabs during an action
      becomes disabling the menu button.
- [x] U3.3 Tiles: three `lv_obj` at 116 px on the surface color, radius 6. Inside each:
      an `lv_chart` (line, no points, no grid, 150 points, shift mode, line 2 px in the
      tile accent, 40 % opacity) filling the tile, a 12 px uppercase label top-left,
      the value (48) with its unit (20, muted) bottom-left. Temperature and pressure
      tiles turn amber (value and curve) while their action runs; the timer tile turns
      amber while a timer runs. Pressure axis fixed 0 to 12 bar, temperature 20 to 160.
- [x] U3.4 Buttons: grid 3 columns x 2 rows, 12 px gaps, stretch to the remaining
      height. Each button: name (20) over value (28). Values: Heat "93 °C", Brew
      "9.0 bar", Prime "7 + 8 s", Steam "135 °C · 4 bar · 4 %" (28 may not fit three
      values; fall back to 20 for that one and check in the simulator), Clean "9 bar",
      Auto "33 s". States: idle (surface, steel border), on (amber fill, dark text;
      steel blue for Steam), disabled (dim text and border).
- [x] U3.5 Notes moved to the header; the bottom panel goes away; the button grid
      takes the space. Timer tile gets "last shot" from `lastBrewTime` when idle (the
      field exists in the state and is unused today; set it when a brew or auto ends).
- [x] U3.6 Simulator pass through every state: idle, heating, brewing, steaming,
      cleaning, prime, auto, and the notes present/absent. Screenshot to
      `docs/images/ui-main.png`.
- [x] U3.7 **(bench)** Flash; bench checklist items 4 to 7; check the curve moves with
      the real pressure reading and that touch targets feel right. Note the `STATUS`
      heap value: the charts and buffers must not move it.

**Checkpoint U3:** the main screen as agreed, on the panel, checklist 4 to 7 passing.

---

## Phase U4: other views, palette only (no bench until the end)

- [x] U4.1 Profiles, Settings, Advanced: apply the theme roles to their existing
      widgets (lists, text areas, keyboard, buttons); fix the one layout that breaks
      with the new fonts if any. The settings tabs keep their layouts.
- [ ] U4.2 (still open, carried to the next branch) The delete-selects-first-profile quirk from the refactor notes: after a
      delete, select the profile that was selected before the deleted one was created
      if it still exists, else the first.
- [x] U4.3 **(bench)** Flash; checklist items 8 and 9.

**Checkpoint U4:** all views in the new palette; settings save and profile operations
unchanged.

---

## Phase U5: wrap-up

- [x] U5.1 `docs/UI.md`: the palette, the roles, how to run the simulator, how to add a
      widget with the theme. README gets the screenshot and a link.
- [x] U5.2 PR `UIv2` to `main`.

## Deferred

- Settings and Advanced view layouts (a form design of their own).
- Non-blocking splash (from the refactor notes).
- Custom font sizes beyond 48 (needs the LVGL font converter; not required now).

## Notes log

- 2026-09-02 merged to main as PR #3 after several simulator/bench rounds: menu instead of
  tabs, tiles with curves, grouped settings, editor overlay, graph view, colors by
  meaning. U5.1 (`docs/UI.md`) folded into the README and the plan; U4.2 stays open.

- 2026-09-02 LVGL memory: adding the graph view's chart series overflowed the 48 KB
  LVGL pool on the board (the assert handler then spins silently: no console output at
  all, the symptom to remember). LVGL now allocates from PSRAM (`LV_MEM_CUSTOM 1`,
  `ps_malloc`; plain `malloc` in the simulator). Static RAM 119 KB -> 33 KB, free heap
  at boot 111 KB -> 152 KB, PSRAM in use by the UI about 80 KB. The draw buffer stays in
  internal RAM. The old `-DLV_MEM_SIZE` override for the simulator is gone.
- 2026-09-02 the controller already reports heater and pump outputs in STAT; the screen
  now keeps them (`boilerOut`, `pumpOut`, `ctrlMode`, `linkOk` in the state) and the
  graph view plots them.

- 2026-09-02 U3: main screen built and rendered in the simulator in the idle and
  brewing scenes (`docs/images/ui-main-idle.png`, `ui-main-brewing.png`). Board build
  752,800 bytes (57 %): the chart widget and the flex/grid layouts cost about 100 KB
  over U2. The simulator got `--scene idle|heating|brewing|steaming` and `--after ms`.
  The perf monitor overlay (FPS/CPU in the corner, on the device too until now) is off.

- 2026-09-02 U1: LVGL's 48 KB pool overflows on the host (64-bit pointers make every
  widget bigger) and the assert handler then spins forever, which looked like a hang.
  `LV_MEM_SIZE` in `lv_conf.h` is now overridable and the simulator passes 512 KB. The
  device keeps 48 KB. `lv_buildUI.c` needed `<stdio.h>`/`<stdlib.h>` (Arduino.h had
  been providing them). The simulator's `--shot` mode uses no SDL at all, so it runs
  from a terminal without a GUI session.

- 2026-09-02: plan written. `docs/images/screen-ui-2025-03-21.png` was an Arduino IDE
  capture, renamed to `arduino-ide-esp32-settings-2025-03.png`; there is no screenshot
  of the old UI, U1.3 makes one from the simulator.
