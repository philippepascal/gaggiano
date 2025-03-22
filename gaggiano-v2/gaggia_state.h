
#ifndef GAGGIA_STATE_H
#define GAGGIA_STATE_H

struct GaggiaState {
  //config and setting based
  bool hasConfigChanged;
  float boilerSetPoint;
  float pressureSetPoint;
  float steamSetPoint;
  float steam_max_pressure;
  float steam_pump_output_percent;
  //real time read
  float tempRead;
  float pressureRead;
  bool isSolenoidOn;
  float lastBrewTime;
  //real time set
  bool hasCommandChanged;
  bool isBoilerOn;
  bool isBrewing;
  bool isSteaming;
  bool isCleaning;
  bool cleanLogs;
};
typedef struct GaggiaState GaggiaStateT;

struct AdvancedSettings {
  bool userChanged;
  bool sendToController;
  double boiler_bb_range;
  double boiler_PID_cycle;
  double boiler_PID_KP;
  double boiler_PID_KI;
  double boiler_PID_KD;
  double pump_max_step_up;
  double pump_KP;
  double pump_KI;
  double pump_KD;
  double unused1;
};
typedef struct AdvancedSettings AdvancedSettingsT;

#endif /*GAGGIA_STATE_H*/