
#ifndef GAGGIA_STATE_H
#define GAGGIA_STATE_H

struct GaggiaState {
  //config and setting based
  bool hasConfigChanged;
  float boilerSetPoint;
  float pressureSetPoint;
  float steamSetPoint;
  //real time read
  float tempRead;
  float pressureRead;
  bool isSolenoidOn;
  //real time set
  bool hasCommandChanged;
  bool isBoilerOn;
  bool isBrewing;
  bool isSteaming;
};
typedef struct GaggiaState GaggiaStateT;

struct AdvancedSettings {
  bool userChanged;
  double boiler_bb_range;
  double boiler_PID_cicle;
  double boiler_PID_KP;
  double boiler_PID_KI;
  double boiler_PID_KD;
  double pump_bb_range;
  double pump_PID_cicle;
  double pump_PID_KP;
  double pump_PID_KI;
  double pump_PID_KD;
};
typedef struct AdvancedSettings AdvancedSettingsT;

#endif /*GAGGIA_STATE_H*/