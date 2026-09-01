// Brew sequencer phase machine (screen): heat, brew, steam, clean, bloom, auto.
#include "check.h"
#include "sequencer.h"
#include "gaggia_protocol.h"
#include <cstring>

static GaggiaStateT S;
static SeqCommand C;

static void reset() {
  std::memset(&S, 0, sizeof(S));
  S.boilerSetPoint = 93; S.pressureSetPoint = 9; S.steamSetPoint = 135;
  S.steam_max_pressure = 4; S.steam_pump_output_percent = 4;
  S.blooming_pressure = 1.5f; S.blooming_fill_time = 7; S.blooming_wait_time = 8; S.brew_timer = 33;
  sequencerReset();
}

static bool press(bool GaggiaState::*flag, bool on, uint32_t now) {
  S.*flag = on;
  S.hasCommandChanged = true;
  return sequencerStep(&S, now, &C);
}

int main() {
  // Heat on: off mode with the boiler setpoint; heat off: all zero.
  reset();
  CHECK(press(&GaggiaState::isBoilerOn, true, 1000));
  CHECK(C.mode == GP_MODE_OFF); CHECK_NEAR(C.tempSet, 93, 0.01); CHECK_NEAR(C.pressSet, 0, 0.01);
  CHECK(!S.hasCommandChanged);
  CHECK(!sequencerStep(&S, 1500, &C));            // nothing to send without a change
  CHECK(press(&GaggiaState::isBoilerOn, false, 2000));
  CHECK(C.mode == GP_MODE_OFF); CHECK_NEAR(C.tempSet, 0, 0.01);

  // Brew with heat on, then off. Timer bookkeeping.
  reset();
  press(&GaggiaState::isBoilerOn, true, 1000);
  CHECK(press(&GaggiaState::isBrewing, true, 5000));
  CHECK(C.mode == GP_MODE_BREW); CHECK_NEAR(C.tempSet, 93, 0.01); CHECK_NEAR(C.pressSet, 9, 0.01);
  CHECK(S.actionStartTime == 5000); CHECK(S.actionStopTime == 0);
  CHECK(press(&GaggiaState::isBrewing, false, 30000));
  CHECK(C.mode == GP_MODE_OFF); CHECK_NEAR(C.tempSet, 93, 0.01);   // heat stays on
  CHECK(S.actionStopTime == 30000);

  // Steam: temp from the steam setpoint, pump percent carried.
  reset();
  CHECK(press(&GaggiaState::isSteaming, true, 1000));
  CHECK(C.mode == GP_MODE_STEAM); CHECK_NEAR(C.tempSet, 135, 0.01);
  CHECK_NEAR(C.pressSet, 4, 0.01); CHECK_NEAR(C.pumpPct, 4, 0.01);

  // Clean: fixed 9 bar.
  reset();
  CHECK(press(&GaggiaState::isCleaning, true, 1000));
  CHECK(C.mode == GP_MODE_CLEAN); CHECK_NEAR(C.pressSet, 9, 0.01);

  // Bloom alone: fill at bloom pressure, then wait with pump off, then done.
  reset();
  press(&GaggiaState::isBoilerOn, true, 0);
  CHECK(press(&GaggiaState::isBlooming, true, 1000));
  CHECK(C.mode == GP_MODE_BREW); CHECK_NEAR(C.pressSet, 1.5, 0.01); CHECK(sequencerPhase() == PHASE_BLOOM_FILL);
  CHECK(!sequencerStep(&S, 7900, &C));              // 6.9 s: still filling
  CHECK(sequencerStep(&S, 8100, &C));               // 7.1 s: wait phase
  CHECK(C.mode == GP_MODE_OFF); CHECK_NEAR(C.tempSet, 93, 0.01); CHECK(sequencerPhase() == PHASE_BLOOM_WAIT);
  CHECK(!sequencerStep(&S, 16000, &C));             // 7.9 s into the wait
  CHECK(!sequencerStep(&S, 16200, &C));             // wait over: bloom ends silently
  CHECK(sequencerPhase() == PHASE_OFF);
  CHECK(!S.isBlooming);
  CHECK(S.actionStartTime == 0);

  // Auto with bloom: fill, wait, brew for brew_timer, then off.
  reset();
  press(&GaggiaState::isBoilerOn, true, 0);
  CHECK(press(&GaggiaState::isAuto, true, 1000));
  CHECK(C.mode == GP_MODE_BREW); CHECK_NEAR(C.pressSet, 1.5, 0.01);
  CHECK(sequencerStep(&S, 8100, &C));               // to wait
  CHECK(C.mode == GP_MODE_OFF);
  CHECK(sequencerStep(&S, 16200, &C));              // to brew
  CHECK(C.mode == GP_MODE_BREW); CHECK_NEAR(C.pressSet, 9, 0.01); CHECK(sequencerPhase() == PHASE_BREW);
  CHECK(!sequencerStep(&S, 16200 + 32900, &C));
  CHECK(sequencerStep(&S, 16200 + 33100, &C));      // brew timer done
  CHECK(C.mode == GP_MODE_OFF); CHECK_NEAR(C.tempSet, 93, 0.01);
  CHECK(!S.isAuto); CHECK(sequencerPhase() == PHASE_OFF);
  CHECK(S.actionStopTime == 16200 + 33100);

  // Auto without bloom configured: straight to brew.
  reset();
  S.blooming_pressure = 0;
  CHECK(press(&GaggiaState::isAuto, true, 1000));
  CHECK(C.mode == GP_MODE_BREW); CHECK_NEAR(C.pressSet, 9, 0.01); CHECK(sequencerPhase() == PHASE_BREW);

  // Auto with no timer: refused, flag cleared, nothing sent.
  reset();
  S.brew_timer = 0;
  CHECK(!press(&GaggiaState::isAuto, true, 1000));
  CHECK(!S.isAuto);

  // Bloom not configured: refused.
  reset();
  S.blooming_wait_time = 0;
  CHECK(!press(&GaggiaState::isBlooming, true, 1000));
  CHECK(!S.isBlooming);

  // Everything off resets the phase.
  reset();
  press(&GaggiaState::isAuto, true, 1000);
  S.isAuto = false;
  S.hasCommandChanged = true;
  CHECK(sequencerStep(&S, 2000, &C));
  CHECK(C.mode == GP_MODE_OFF); CHECK(sequencerPhase() == PHASE_OFF);

  return test_summary("test_sequencer");
}
