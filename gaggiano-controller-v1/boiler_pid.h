// Boiler heater control: bang-bang outside a band, PID inside it, plus a
// feed-forward for the cold water the pump pushes in during a brew. Plain C++
// so the host tests cover it (tests/test_boiler_pid.cpp).
//
// Same shape as the AutoPID library it replaces (2026-09-06), with three fixes:
// the derivative is taken on the measurement in degrees per second (the library
// divided by the step twice and its Kd never did anything), the integral is
// clamped to what the output can use (no wind-up during a long dip), and a step
// after a bang-bang phase starts from the current reading instead of a stale one.
#pragma once
#include <stdint.h>

struct BoilerPidParams {
  float kp;         // percent per degree
  float ki;         // percent per degree-second
  float kd;         // percent per (degree per second), on the measurement
  float bangOn;     // degrees under the setpoint beyond which the output is outMax; 0 disables
  float bangOff;    // degrees over the setpoint beyond which the output is outMin; 0 disables
  float outMin;
  float outMax;
  uint32_t stepMs;  // PID recomputed at most this often; bang-bang acts on every call
};

struct BoilerPid {
  uint32_t lastStep = 0;
  float integral = 0;
  float prevError = 0;
  float prevInput = 0;
  bool inPid = false;  // the previous call ran the PID branch (prevError/prevInput are valid)
  float output = 0;
};

void boilerPidReset(BoilerPid *s, uint32_t now);

// Output in [outMin, outMax]. Between PID steps the previous output is returned.
float boilerPidStep(BoilerPid *s, const BoilerPidParams *p, uint32_t now, float setpoint, float input);

// Heater percent that warms the incoming water: pumpFraction (0..1 of the pump
// range) times the pump's flow at full range, times the rise from inlet to the
// setpoint. Capped at maxPct; the PID trims whatever this estimate gets wrong.
float brewBoostPercent(float pumpFraction, float flowMlS, float tempSet, float inletC, float elementW,
                       float maxPct);
