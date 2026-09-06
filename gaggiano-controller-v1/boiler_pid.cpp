#include "boiler_pid.h"

void boilerPidReset(BoilerPid *s, uint32_t now) {
  s->lastStep = now;
  s->integral = 0;
  s->prevError = 0;
  s->prevInput = 0;
  s->inPid = false;
  s->output = 0;
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

float boilerPidStep(BoilerPid *s, const BoilerPidParams *p, uint32_t now, float setpoint, float input) {
  float error = setpoint - input;
  if (p->bangOn > 0 && error > p->bangOn) {
    s->output = p->outMax;
    s->lastStep = now;
    s->inPid = false;
    return s->output;
  }
  if (p->bangOff > 0 && -error > p->bangOff) {
    s->output = p->outMin;
    s->lastStep = now;
    s->inPid = false;
    return s->output;
  }
  uint32_t dtMs = now - s->lastStep;
  if (dtMs < p->stepMs) return s->output;
  s->lastStep = now;
  float dt = dtMs / 1000.0f;
  if (!s->inPid) {
    s->prevError = error;
    s->prevInput = input;
    s->inPid = true;
  }
  s->integral += (error + s->prevError) * 0.5f * dt;
  if (p->ki > 0) s->integral = clampf(s->integral, p->outMin / p->ki, p->outMax / p->ki);
  float derivative = -(input - s->prevInput) / dt;  // on the measurement: no kick on a setpoint change
  s->prevError = error;
  s->prevInput = input;
  s->output = clampf(p->kp * error + p->ki * s->integral + p->kd * derivative, p->outMin, p->outMax);
  return s->output;
}

float brewBoostPercent(float pumpFraction, float flowMlS, float tempSet, float inletC, float elementW,
                       float maxPct) {
  if (pumpFraction <= 0 || flowMlS <= 0 || elementW <= 0) return 0;
  float rise = tempSet - inletC;
  if (rise <= 0) return 0;
  float watts = pumpFraction * flowMlS * 4.186f * rise;
  float pct = watts / elementW * 100.0f;
  return pct > maxPct ? maxPct : pct;
}
