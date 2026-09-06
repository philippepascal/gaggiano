#include "control.h"
#include "config.h"
#include "sensors.h"
#include "outputs.h"
#include "steam_assist.h"
#include "boiler_pid.h"

double operating_mode = OPERATING_MODE_OFF;
double temperatureSetPoint = 0;
double pressureSetPoint = 0;
double pressureOutputPercent = 0;

double boiler_bb_range = 3;
double boiler_PID_cycle = 1000;
double boiler_PID_KP = 10;
double boiler_PID_KI = 0.2;
double boiler_PID_KD = 0.1;
double pump_max_step_up = 0.2;
double pump_KP = 1;
double pump_KI = 1.7;
double pump_KD = 0.9;
double pump_flow_ml_s = 9;
double steam_shot_s = 0.15;
double steam_gap_s = 2;
double steam_min_temp = 130;

static SteamAssist steamAssist;
static BoilerPid boilerPid;

void controlSetup() {
  boilerPidReset(&boilerPid, millis());
}

// Steaming with the wand open (pressure under the max) is a large heat sink: the
// proportional band alone lets the boiler slide 8 degrees before the heater goes
// flat out (2026-09-05 log). Narrow the "flat out below" side of the bang-bang
// then; the "off above" side and the PID inside the band are unchanged, and the
// wand closed brings the normal band back.
//
// Brewing pushes reservoir water into the boiler; the PID only sees that once the
// reading has dropped (2026-09-06 log: 3 to 4 degrees under for 25 s of a 33 s
// shot, heater at 28 percent, which is what that water takes). The feed-forward
// adds that estimate while the pump runs; the PID trims what it gets wrong.
void updateBoiler() {
  bool wandOpen = operating_mode == OPERATING_MODE_STEAM && pressure_smoothed < pressureSetPoint;
  BoilerPidParams p;
  p.kp = boiler_PID_KP;
  p.ki = boiler_PID_KI;
  p.kd = boiler_PID_KD;
  p.bangOn = wandOpen ? STEAM_OPEN_BB_RANGE : boiler_bb_range;
  p.bangOff = boiler_bb_range;
  p.outMin = BOILER_OUTPUT_MIN;
  p.outMax = BOILER_OUTPUT_MAX;
  p.stepMs = (uint32_t)boiler_PID_cycle;
  float out = boilerPidStep(&boilerPid, &p, millis(), temperatureSetPoint, temperature_smoothed);
  if (operating_mode == OPERATING_MODE_BREW && pump_dimmer_output2 > 0) {
    out += brewBoostPercent(pump_dimmer_output2 / PUMP_RANGE, pump_flow_ml_s, temperatureSetPoint, BREW_INLET_C,
                            BOILER_ELEMENT_W, BREW_BOOST_MAX_PCT);
    if (out > BOILER_OUTPUT_MAX) out = BOILER_OUTPUT_MAX;
  }
  boiler_relay_output = out;
  setBoilerOutput(boiler_relay_output);
}

// The pump never runs on a pressure the controller does not have: with the sensor
// dead the pump stays off in every mode (the valve still follows the mode).
static void setPumpGuarded(double value) {
  setPump(pressureStale ? 0 : value);
}

void updatePump2() {
  double pumpValue;
  if (operating_mode != OPERATING_MODE_STEAM) steamAssistReset(&steamAssist);
  if (operating_mode == OPERATING_MODE_OFF) {
    setPump(0);
    setValve(false);
  } else if (operating_mode == OPERATING_MODE_BREW) {
    if (pressureSetPoint > 0) {
      setValve(true);
      if (pressure_smoothed > pressureSetPoint) {
        pumpValue = 0;
      } else {
        float diff = pressureSetPoint - pressure_smoothed;
        pumpValue = PUMP_RANGE / (pump_KP + exp(pump_KI - diff / pump_KD));
        if ((pressure_smoothed < (pressureSetPoint / 2)) && ((pumpValue - pump_dimmer_output2) > pump_max_step_up)) {  // should only happen for low pressures...
          pumpValue = pump_dimmer_output2 + pump_max_step_up;
        }
      }
      setPumpGuarded(pumpValue);
    } else {
      setPump(0);
      setValve(false);
    }
  } else if (operating_mode == OPERATING_MODE_CLEAN) {
    if (pressureSetPoint > 0) {
      setValve(true);
      setPumpGuarded(pressure_smoothed > pressureSetPoint ? 0 : PUMP_MAX);
    } else {
      setPump(0);
      setValve(false);
    }
  } else if (operating_mode == OPERATING_MODE_STEAM) {
    SteamAssistParams p;
    p.pumpPct = pressureOutputPercent;
    p.maxPct = STEAM_PUMP_MAX_PERCENT;
    p.maxPressure = pressureSetPoint;
    p.shotS = steam_shot_s;
    p.gapS = steam_gap_s;
    p.minTemp = steam_min_temp;
    p.blankS = STEAM_ASSIST_BLANK_S;
    pumpValue = steamAssistStep(&steamAssist, &p, millis(), temperature_smoothed, pressure_smoothed, PUMP_RANGE);
    setPumpGuarded(pumpValue);
  } else {
    // safety. should not happen
    setPump(0);
    setValve(false);
  }
}

void allOutputsOff() {
  operating_mode = OPERATING_MODE_OFF;
  setPump(0);
  setValve(false);
  setBoilerOutput(0);
  temperatureSetPoint = 0;
  pressureSetPoint = 0;
  pressureOutputPercent = 0;
}
