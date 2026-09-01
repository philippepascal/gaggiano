#include "control.h"
#include "config.h"
#include "sensors.h"
#include "outputs.h"
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

void updatePump2() {
  double pumpValue;
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
      setPump(pumpValue);
    } else {
      setPump(0);
      setValve(false);
    }
  } else if (operating_mode == OPERATING_MODE_CLEAN) {
    if (pressureSetPoint > 0) {
      setValve(true);
      setPump(pressure_smoothed > pressureSetPoint ? 0 : PUMP_MAX);
    } else {
      setPump(0);
      setValve(false);
    }
  } else if (operating_mode == OPERATING_MODE_STEAM) {
    if (pressure_smoothed > pressureSetPoint) {
      pumpValue = 0;
    } else {
      float p = pressureOutputPercent;
      if (pressureOutputPercent > 10) {  // just safety, solenoid is closed!
        p = 10;
      }
      pumpValue = (p * PUMP_RANGE) / 100;
    }
    setPump(pumpValue);
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
