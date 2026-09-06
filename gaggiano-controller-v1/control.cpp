#include "control.h"
#include "config.h"
#include "sensors.h"
#include "outputs.h"
#include "steam_assist.h"
#include <AutoPID.h>

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
double unused1 = 0;
double steam_shot_s = 0.15;
double steam_gap_s = 2;

static SteamAssist steamAssist;

// input/output variables passed by reference, so they are updated automatically
static AutoPID boilerPID(&temperature_smoothed, &temperatureSetPoint, &boiler_relay_output,
                         BOILER_OUTPUT_MIN, BOILER_OUTPUT_MAX, BOILER_KP, BOILER_KI, BOILER_KD);

void controlSetup() {
  // if temperature is more than bb_range below or above setpoint, output is min or max
  boilerPID.setBangBang(boiler_bb_range);
  boilerPID.setTimeStep(boiler_PID_cycle);
}

void updateAdvancedSettings() {
  boilerPID.setBangBang(boiler_bb_range);
  boilerPID.setTimeStep(boiler_PID_cycle);
  boilerPID.setGains(boiler_PID_KP, boiler_PID_KI, boiler_PID_KD);
}

void updateBoiler() {
  boilerPID.run();
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
    p.tempMargin = STEAM_ASSIST_TEMP_MARGIN;
    p.blankS = STEAM_ASSIST_BLANK_S;
    pumpValue = steamAssistStep(&steamAssist, &p, millis(), temperature_smoothed, temperatureSetPoint,
                                pressure_smoothed, PUMP_RANGE);
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
