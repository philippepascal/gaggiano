// Boiler temperature (MAX6675) and pressure (ADS1115), Kalman-smoothed.
#pragma once
#include <Arduino.h>

extern double temperature_read;
extern double temperature_smoothed;
extern double pressure_read;
extern double pressure_smoothed;
extern uint32_t temperatureFaults;  // readings rejected (open thermocouple reads 0 or NaN)

void sensorsSetup();
bool readTemperature(uint32_t now);  // true when a new reading was taken
bool readPressure(uint32_t now);
