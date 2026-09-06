// Steam assist: while steaming, top the boiler up with short pump shots so the
// pressure holds, without pushing water through the wand. Plain C++ so the host
// tests cover it; the controller calls stepSteamAssist every pressure read.
//
// A shot fires only when all of these hold:
//   - the wand is open: pressure below maxPressure (closed, the boiler sits well
//     above it on saturated steam, so this also stops any shot into a shut boiler)
//   - the element has spare capacity: temperature within tempMargin of the setpoint
//     (below it the element is already flat out and water would only make wet steam)
//   - the previous shot ended at least max(gapS, blankS) ago; the pressure is not
//     looked at for blankS after a shot because the pump's own pulses raise it
// A shot is a fixed shotS of pump at pumpPct (capped at maxPct) and is never cut
// short: the quarter gram it delivers is harmless even if the wand shuts meanwhile.
#pragma once
#include <stdint.h>

struct SteamAssistParams {
  float pumpPct;      // pump level during a shot, percent of the pump range; 0 disables
  float maxPct;       // hard cap on pumpPct (solenoid closed)
  float maxPressure;  // bar; the wand counts as open below this
  float shotS;        // seconds of pump per shot; 0 disables
  float gapS;         // seconds between the end of a shot and the next one
  float tempMargin;   // degrees below the setpoint where shots are still allowed
  float blankS;       // seconds after a shot during which the pressure is ignored
};

struct SteamAssist {
  bool shotOn = false;
  uint32_t shotStart = 0;
  bool hadShot = false;
  uint32_t lastShotEnd = 0;
};

void steamAssistReset(SteamAssist *a);

// Pump level in [0, range] for this pass. `now` in ms.
float steamAssistStep(SteamAssist *a, const SteamAssistParams *p, uint32_t now, float temp, float tempSet,
                      float pressure, float range);
