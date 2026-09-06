// Steam assist (controller): shots of pump while steaming, gated on wand open
// (pressure), boiler at temperature, and a pause between shots.
#include "check.h"
#include "steam_assist.h"

static SteamAssist A;
static SteamAssistParams P;

static void reset() {
  steamAssistReset(&A);
  P.pumpPct = 20; P.maxPct = 50; P.maxPressure = 1.0f;
  P.shotS = 0.15f; P.gapS = 2; P.tempMargin = 2; P.blankS = 0.3f;
}
// 127 is the pump range; 20% of it is 25.4
static float step(uint32_t now, float temp, float pressure) {
  return steamAssistStep(&A, &P, now, temp, 135, pressure, 127);
}

int main() {
  // Wand open, boiler hot: a shot starts at once and lasts shotS, then stops.
  reset();
  CHECK_NEAR(step(1000, 135, 0.4f), 25.4, 0.01);
  CHECK_NEAR(step(1100, 135, 0.4f), 25.4, 0.01);
  CHECK_NEAR(step(1140, 135, 3.0f), 25.4, 0.01);   // not cut short by its own pressure spike
  CHECK_NEAR(step(1150, 135, 0.4f), 0, 0.01);      // over
  // Pause: nothing for gapS after the shot ended, even with everything else true.
  CHECK_NEAR(step(2000, 135, 0.4f), 0, 0.01);
  CHECK_NEAR(step(3140, 135, 0.4f), 0, 0.01);
  CHECK_NEAR(step(3150, 135, 0.4f), 25.4, 0.01);   // gap elapsed: next shot
  CHECK(A.shotOn);

  // Wand closed (pressure at or above the max) never fires.
  reset();
  CHECK_NEAR(step(1000, 135, 1.0f), 0, 0.01);
  CHECK_NEAR(step(1010, 135, 2.1f), 0, 0.01);
  CHECK(!A.shotOn);
  CHECK_NEAR(step(1020, 135, 0.9f), 25.4, 0.01);

  // Boiler sagging below the margin: no shot; back within the margin: shot.
  reset();
  CHECK_NEAR(step(1000, 132.9f, 0.4f), 0, 0.01);
  CHECK_NEAR(step(1010, 133.0f, 0.4f), 25.4, 0.01);

  // Disabled by a zero pump percent or a zero shot length.
  reset(); P.pumpPct = 0;
  CHECK_NEAR(step(1000, 135, 0.4f), 0, 0.01);
  reset(); P.shotS = 0;
  CHECK_NEAR(step(1000, 135, 0.4f), 0, 0.01);

  // The pump percent is capped at maxPct.
  reset(); P.pumpPct = 80;
  CHECK_NEAR(step(1000, 135, 0.4f), 63.5, 0.01);

  // A gap shorter than the blanking still waits for the blanking.
  reset(); P.gapS = 0.1f;
  CHECK_NEAR(step(1000, 135, 0.4f), 25.4, 0.01);
  CHECK_NEAR(step(1150, 135, 0.4f), 0, 0.01);      // shot over at 1150
  CHECK_NEAR(step(1250, 135, 0.4f), 0, 0.01);      // gap elapsed, blanking not
  CHECK_NEAR(step(1450, 135, 0.4f), 25.4, 0.01);

  // Reset forgets the pause: leaving and re-entering steam starts fresh.
  reset();
  step(1000, 135, 0.4f); step(1150, 135, 0.4f);
  steamAssistReset(&A);
  CHECK_NEAR(step(1200, 135, 0.4f), 25.4, 0.01);

  return test_summary("test_steam_assist");
}
