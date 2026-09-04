// Boiler temperature (MAX6675) and pressure (ADS1115), Kalman-smoothed.
#pragma once
#include <Arduino.h>

extern double temperature_read;
extern double temperature_smoothed;
extern double pressure_read;
extern double pressure_smoothed;
extern uint32_t temperatureFaults;  // readings rejected (open thermocouple reads 0 or NaN)
extern uint32_t pressureFaults;     // I2C reads that failed (value kept from the last good one)
extern uint32_t i2cRecoveries;      // bus recoveries attempted after repeated pressure faults
extern bool pressureStale;          // no good pressure reading for PRESSURE_STALE_MS: pump must not run on it

void sensorsSetup();
bool readTemperature(uint32_t now);  // true when a new reading was taken
bool readPressure(uint32_t now);     // true when a read was attempted (see pressureStale)
