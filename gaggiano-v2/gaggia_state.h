
#ifndef GAGGIA_STATE_H
#define GAGGIA_STATE_H

struct GaggiaState {
  //config and setting based
  bool hasChanged;
  float boilerSetPoint;
  float pressureSetPoint;
  float steamSetPoint;
  //real time
  float tempRead;
  float pressureRead;
  bool isBoilerOn;
  bool isBrewing;
  bool isSteaming;
};
typedef struct GaggiaState GaggiaStateT;

#endif /*GAGGIA_STATE_H*/