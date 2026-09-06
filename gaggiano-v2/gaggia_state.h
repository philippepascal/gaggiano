#ifndef GAGGIA_STATE_H
#define GAGGIA_STATE_H

#define PROFILE_NAME_MAX 32  // including the terminator; SD filename budget (D3: 24 chars + ".csv")
#define NOTES_MAX 128        // including the terminator

struct GaggiaState {
  //config and setting based
  bool hasConfigChanged;
  float boilerSetPoint;
  float pressureSetPoint;
  float steamSetPoint;
  float steam_max_pressure;
  float steam_pump_output_percent;
  float steam_shot_s;  // steam assist: seconds of pump per shot
  float steam_gap_s;   // steam assist: seconds between shots
  float blooming_pressure;
  float blooming_fill_time;
  float blooming_wait_time;
  float brew_timer;
  //profile meta data
  bool hasMetaDataChanged;
  char profile_name[PROFILE_NAME_MAX];
  char notes[NOTES_MAX];
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
  bool isBlooming;
  bool isAuto;
  bool isCleaning;
  bool cleanLogs;
  int actionStartTime;
  int actionStopTime;
  //controller outputs, from the status line
  float boilerOut;   // heater duty, percent
  float pumpOut;     // pump level, 0..127
  int ctrlMode;      // GP_MODE_*
  bool linkOk;       // the controller considers the link alive
  bool ctrlAlive;    // STAT lines are arriving (set by the link every pass)
  bool pressStale;   // the controller has no pressure reading (pump held off)
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
