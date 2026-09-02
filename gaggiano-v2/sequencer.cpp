#include "sequencer.h"
#include <gaggia_protocol.h>

static int currentPhase = PHASE_OFF;
static uint32_t phaseStart = 0;  // phase durations are measured from here; the
                                 // on-screen timer (actionStartTime) runs across phases

int sequencerPhase() { return currentPhase; }
void sequencerReset() { currentPhase = PHASE_OFF; }

// "Brew" as the old firmware called it: pump to a pressure when pressure > 0,
// otherwise everything off with the boiler holding temp.
static bool brew(SeqCommand *out, float temp, float pressure) {
  out->mode = pressure > 0 ? GP_MODE_BREW : GP_MODE_OFF;
  out->tempSet = temp;
  out->pressSet = pressure;
  out->pumpPct = 0;
  return true;
}

static bool clean(SeqCommand *out, float temp, float pressure) {
  out->mode = GP_MODE_CLEAN;
  out->tempSet = temp;
  out->pressSet = pressure;
  out->pumpPct = 0;
  return true;
}

static bool steam(SeqCommand *out, float temp, float maxPressure, float pumpPct) {
  out->mode = GP_MODE_STEAM;
  out->tempSet = temp;
  out->pressSet = maxPressure;
  out->pumpPct = pumpPct;
  return true;
}

static bool bloomConfigured(const GaggiaStateT *s) {
  return s->blooming_fill_time > 0 && s->blooming_wait_time > 0 && s->blooming_pressure > 0;
}

static void markStopped(GaggiaStateT *s, uint32_t now) {
  if (s->actionStartTime > 0 && s->actionStopTime == 0) s->actionStopTime = (int)now;
}

static void markStarted(GaggiaStateT *s, uint32_t now) {
  s->actionStartTime = (int)now;
  s->actionStopTime = 0;
  phaseStart = now;
}

bool sequencerStep(GaggiaStateT *s, uint32_t now, SeqCommand *out) {
  float temp = 0;
  if (s->isBoilerOn) {
    temp = s->boilerSetPoint;
  } else if (s->isSteaming) {
    temp = s->steamSetPoint;
  }

  if (s->hasCommandChanged) {
    s->hasCommandChanged = false;
    if (s->isBrewing) {
      markStarted(s, now);
      return brew(out, temp, s->pressureSetPoint);
    }
    if (s->isCleaning) {
      markStarted(s, now);
      return clean(out, temp, 9);
    }
    if (s->isBlooming) {
      if (currentPhase == PHASE_OFF) {
        if (bloomConfigured(s)) {
          currentPhase = PHASE_BLOOM_FILL;
          markStarted(s, now);
          return brew(out, temp, s->blooming_pressure);
        }
        s->isBlooming = false;
      }
      return false;
    }
    if (s->isAuto) {
      if (currentPhase == PHASE_OFF) {
        if (s->brew_timer > 0) {
          markStarted(s, now);
          if (bloomConfigured(s)) {
            currentPhase = PHASE_BLOOM_FILL;
            return brew(out, temp, s->blooming_pressure);
          }
          currentPhase = PHASE_BREW;
          return brew(out, temp, s->pressureSetPoint);
        }
        s->isAuto = false;
      }
      return false;
    }
    if (s->isSteaming) {
      markStopped(s, now);
      return steam(out, s->steamSetPoint, s->steam_max_pressure, s->steam_pump_output_percent);
    }
    if (s->isBoilerOn) {
      markStopped(s, now);
      return brew(out, s->boilerSetPoint, 0);
    }
    markStopped(s, now);
    currentPhase = PHASE_OFF;
    return brew(out, 0, 0);
  }

  // No button change: advance the timed phases. The action timer keeps running
  // across fill, wait and brew and holds the total once the sequence ends.
  uint32_t elapsed = now - phaseStart;
  if (currentPhase == PHASE_BLOOM_FILL) {
    if (elapsed > (uint32_t)(s->blooming_fill_time * 1000)) {
      currentPhase = PHASE_BLOOM_WAIT;
      phaseStart = now;
      return brew(out, temp, 0);
    }
  } else if (currentPhase == PHASE_BLOOM_WAIT) {
    if (elapsed > (uint32_t)(s->blooming_wait_time * 1000)) {
      if (s->isBlooming) {
        currentPhase = PHASE_OFF;
        s->actionStopTime = (int)now;
        s->isBlooming = false;
      } else if (s->isAuto) {
        currentPhase = PHASE_BREW;
        phaseStart = now;
        return brew(out, temp, s->pressureSetPoint);
      }
    }
  } else if (currentPhase == PHASE_BREW) {
    if (elapsed > (uint32_t)(s->brew_timer * 1000)) {
      currentPhase = PHASE_OFF;
      s->actionStopTime = (int)now;
      s->isAuto = false;
      return brew(out, temp, 0);
    }
  }
  return false;
}
