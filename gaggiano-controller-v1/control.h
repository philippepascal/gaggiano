// Operating mode, setpoints, tuning parameters, and the control laws.
#pragma once
#include <Arduino.h>

extern double operating_mode;  // OPERATING_MODE_*
extern double temperatureSetPoint;
extern double pressureSetPoint;
extern double pressureOutputPercent;  // steam: pump duty cap in percent

extern double boiler_bb_range;
extern double boiler_PID_cycle;
extern double boiler_PID_KP;
extern double boiler_PID_KI;
extern double boiler_PID_KD;
extern double pump_max_step_up;
extern double pump_KP;
extern double pump_KI;
extern double pump_KD;
extern double unused1;

void controlSetup();
void updateBoiler();             // call after each temperature reading
void updatePump2();              // call after each pressure reading
void updateAdvancedSettings();   // apply the tuning parameters to the PID
void allOutputsOff();            // pump 0, valve closed, boiler 0, setpoints 0
