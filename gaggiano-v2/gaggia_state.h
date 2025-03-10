
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

#endif /*GAGGIA_STATE_H*/