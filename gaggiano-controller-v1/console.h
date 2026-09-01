// USB serial console (from the Mac, not the screen). One command per line:
// VERSION, STATUS, LOG ON, LOG OFF, RX <protocol line>, HANG, DFU. Non-blocking.
#pragma once
#include <Arduino.h>

extern bool debugLog;        // LOG ON / LOG OFF: echo link traffic on the console
extern uint32_t loopCounter;  // owned by the sketch
extern uint32_t maxLoopMs;
extern bool resetByWatchdog;  // last boot was a watchdog reset

void consoleSetup();
void readUsbCommand();
void printStatus();
void printSetPoints();
void printAdvancedSettings();
