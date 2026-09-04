#include "sequencer.h"
#include <gaggia_protocol.h>

static int currentPhase = PHASE_OFF;
static bool brewRunning = false;  // pump at brew pressure, manual or the auto brew phase
static uint32_t brewStart = 0;    // when it started; the duration becomes lastBrewTime
static uint32_t phaseStart = 0;   // phase durations are measured from here; the
                                  // on-screen timer (actionStartTime) runs across phases

int sequencerPhase() { return currentPhase; }
void sequencerReset() {
  currentPhase = PHASE_OFF;
  brewRunning = false;
}

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
bool sequencerBloomConfigured(const GaggiaStateT *s) { return bloomConfigured(s); }

static void markStopped(GaggiaStateT *s, uint32_t now) {
  if (s->actionStartTime > 0 && s->actionStopTime == 0) s->actionStopTime = (int)now;
}

// The last shot is the brew phase alone: bloom fill and wait do not count, and an
// auto run stopped before its brew leaves the previous value.
static void startBrew(uint32_t now) {
  brewStart = now;
  brewRunning = true;
}

static void rememberShot(GaggiaStateT *s, uint32_t now) {
  if (!brewRunning) return;
  brewRunning = false;
  s->lastBrewTime = (now - brewStart) / 1000.0f;
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
      startBrew(now);
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
          startBrew(now);
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
    // Nothing runs any more: a manual brew ended, or a prime/auto was interrupted.
    rememberShot(s, now);
    markStopped(s, now);
    currentPhase = PHASE_OFF;
    return brew(out, s->isBoilerOn ? s->boilerSetPoint : 0, 0);
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
        startBrew(now);
        return brew(out, temp, s->pressureSetPoint);
      }
    }
  } else if (currentPhase == PHASE_BREW) {
    if (elapsed > (uint32_t)(s->brew_timer * 1000)) {
      currentPhase = PHASE_OFF;
      s->actionStopTime = (int)now;
      s->isAuto = false;
      rememberShot(s, now);
      return brew(out, temp, 0);
    }
  }
  return false;
}
