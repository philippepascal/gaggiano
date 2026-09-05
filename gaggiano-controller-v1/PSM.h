#ifndef PSM_h
#define PSM_h

#include "Arduino.h"

class PSM
{
public:
  PSM(unsigned char sensePin, unsigned char controlPin, unsigned int range, int mode = RISING, unsigned char divider = 1, unsigned char interruptMinTimeDiff = 0);

  void set(unsigned int value);

  long getCounter();
  void resetCounter();

  void stopAfter(long counter);

  unsigned int cps();
  unsigned long getLastMillis();

  unsigned char getDivider(void);
  void setDivider(unsigned char divider = 1);
  void shiftDividerCounter(char value = 1);

private:
  static inline void onInterrupt();
  void calculateSkip();
  void updateControl();

  unsigned char _sensePin = 0;
  unsigned char _controlPin = 0;
  unsigned int _range = 0;
  unsigned char _divider = 1;
  unsigned char _dividerCounter = 1;
  unsigned char _interruptMinTimeDiff = 0;
  // Every member is initialised here: the object lives on the heap ('new' in
  // outputsSetup) and the heap is not zeroed at boot, only .bss is. Left
  // indeterminate, _stopAfter/_counter came up with whatever the SRAM held and
  // on some boots the ISR took the "stopAfter reached" branch on the first
  // zero-cross: _skip stayed true, the pump never ran until a lucky restart
  // (2026-09-05).
  volatile unsigned int _value = 0;
  volatile unsigned int _a = 0;
  volatile bool _skip = true;
  volatile long _counter = 0;
  volatile long _stopAfter = 0;
  volatile unsigned long _lastMillis = 0;
};

extern PSM* _thePSM;

#endif
