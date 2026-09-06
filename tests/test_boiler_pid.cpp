// Boiler heater control (controller): bang-bang band, PID inside it, brew feed-forward.
#include "check.h"
#include "boiler_pid.h"

static BoilerPid S;
static BoilerPidParams P;

static void reset() {
  boilerPidReset(&S, 0);
  P.kp = 5; P.ki = 0.1f; P.kd = 0;
  P.bangOn = 10; P.bangOff = 10;
  P.outMin = 0; P.outMax = 100;
  P.stepMs = 200;
}
static float step(uint32_t now, float input) { return boilerPidStep(&S, &P, now, 80, input); }

int main() {
  // Bang-bang: more than the band under -> max, more than the band over -> min, on every call.
  reset();
  CHECK_NEAR(step(10, 60), 100, 0.001);
  CHECK_NEAR(step(20, 69.9f), 100, 0.001);
  CHECK_NEAR(step(30, 95), 0, 0.001);
  CHECK_NEAR(step(40, 90.1f), 0, 0.001);

  // Inside the band: P and I as the AutoPID library computed them at today's gains
  // (trapezoid integral, first step rectangular), recomputed only every stepMs.
  reset();
  CHECK_NEAR(step(200, 75), 25.1, 0.01);   // err 5: P 25, I 0.1 * (5 * 0.2)
  CHECK_NEAR(step(300, 76), 25.1, 0.01);   // too early: previous output
  CHECK_NEAR(step(400, 76), 20.19, 0.01);  // err 4: P 20, I 0.1 * (1.0 + (4+5)/2 * 0.2)

  // Derivative on the measurement: a rising temperature pulls the output down,
  // a falling one pushes it up, and a setpoint change alone does not kick.
  reset(); P.kd = 3;
  step(200, 75);
  CHECK_NEAR(step(400, 76), 20.19 - 15, 0.01);   // +1 degree in 0.2 s = 5 deg/s -> -15
  CHECK_NEAR(step(600, 75), 25.28 + 15, 0.01);   // err 5: I 1.9 + 0.9 = 2.8 -> P 25 + I 0.28, D +15
  {
    BoilerPid s2; boilerPidReset(&s2, 0);
    boilerPidStep(&s2, &P, 200, 80, 78);
    float a = boilerPidStep(&s2, &P, 400, 80, 78);
    float b = boilerPidStep(&s2, &P, 600, 82, 78);   // setpoint moved, reading did not
    CHECK_NEAR(b - a, 10 + 0.1f * (3 * 0.2f + 1 * 0.2f), 0.05);  // only P and I moved
  }

  // Coming out of bang-bang starts from the current reading: no derivative spike.
  reset(); P.kd = 3;
  step(0, 60);                              // bang-bang
  CHECK_NEAR(step(200, 72), 5 * 8 + 0.1f * (8 * 0.2f), 0.01);

  // Anti-windup: the integral never exceeds what the output can use, either way.
  reset();
  for (uint32_t t = 200; t <= 400000; t += 200) step(t, 71);   // err 9 for 400 s
  CHECK(S.integral <= 100 / 0.1f + 0.001f);
  CHECK_NEAR(step(400200, 71), 100, 0.001);
  for (uint32_t t = 400400; t <= 800000; t += 200) step(t, 89); // err -9 for 400 s
  CHECK(S.integral >= -0.001f);
  CHECK_NEAR(step(800200, 89), 0, 0.001);

  // Brew feed-forward: 17/127 of the pump, 9 ml/s at full, 20 -> 80 degrees, 1400 W.
  CHECK_NEAR(brewBoostPercent(17.0f / 127, 9, 80, 20, 1400, 40), 21.6, 0.2);
  CHECK_NEAR(brewBoostPercent(1, 9, 93, 20, 1400, 40), 40, 0.001);          // capped
  CHECK_NEAR(brewBoostPercent(0, 9, 80, 20, 1400, 40), 0, 0.001);
  CHECK_NEAR(brewBoostPercent(0.2f, 0, 80, 20, 1400, 40), 0, 0.001);       // flow 0 disables
  CHECK_NEAR(brewBoostPercent(0.2f, 9, 0, 20, 1400, 40), 0, 0.001);        // boiler off

  return test_summary("test_boiler_pid");
}
