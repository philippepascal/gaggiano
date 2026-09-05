// Boiler SSR (timer PWM), pump (pulse-skip modulation), 3-way solenoid valve.
#pragma once
#include <Arduino.h>

extern double boiler_relay_output;  // 0..100, written by the PID
extern double pump_dimmer_output2;  // 0..PUMP_RANGE
extern volatile uint32_t zeroCrossings;  // mains half-cycles seen on PUMP_ZC_PIN; static = the dimmer's zero-cross signal is missing, the pump can never run

void outputsSetup();
void setBoilerOutput(double percent);
void setPump(double value);
void setValve(bool open);
bool valveIsOpen();
