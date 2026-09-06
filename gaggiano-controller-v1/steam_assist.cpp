#include "steam_assist.h"

void steamAssistReset(SteamAssist *a) {
  a->shotOn = false;
  a->shotStart = 0;
  a->hadShot = false;
  a->lastShotEnd = 0;
}

static float shotLevel(const SteamAssistParams *p, float range) {
  float pct = p->pumpPct;
  if (pct > p->maxPct) pct = p->maxPct;
  if (pct < 0) pct = 0;
  return pct * range / 100.0f;
}

float steamAssistStep(SteamAssist *a, const SteamAssistParams *p, uint32_t now, float temp, float pressure,
                      float range) {
  if (a->shotOn) {
    if ((now - a->shotStart) >= (uint32_t)(p->shotS * 1000.0f)) {
      a->shotOn = false;
      a->hadShot = true;
      a->lastShotEnd = now;
      return 0;
    }
    return shotLevel(p, range);
  }
  if (p->pumpPct <= 0 || p->shotS <= 0) return 0;
  if (a->hadShot) {
    float pause = p->gapS > p->blankS ? p->gapS : p->blankS;
    if ((now - a->lastShotEnd) < (uint32_t)(pause * 1000.0f)) return 0;
  }
  if (pressure >= p->maxPressure) return 0;        // wand closed
  if (temp < p->minTemp) return 0;                 // boiler too cold, element already flat out
  a->shotOn = true;
  a->shotStart = now;
  return shotLevel(p, range);
}
