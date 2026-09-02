#include "sensors.h"
#include "config.h"
#include <Wire.h>
#include <ADS1X15.h>
#include <max6675.h>
#include <SimpleKalmanFilter.h>

double temperature_read = 0;
double temperature_smoothed = 0;
double pressure_read = 0;
double pressure_smoothed = 0;
uint32_t temperatureFaults = 0;

static ADS1115 ADS;
static MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);
static SimpleKalmanFilter smoothPressure(0.6f, 0.6f, 0.1f);
static SimpleKalmanFilter smoothTemperature(0.25f, 0.25f, 0.01f);

static uint32_t last_temp_read_time = 0;
static uint32_t last_pressure_read_time = 0;

void sensorsSetup() {
  Wire.setSDA(I2C_SDA_PIN);  // should not be necessary.. default value
  Wire.setSCL(I2C_SCL_PIN);  // should not be necessary.. default value
  ADS = ADS1115(ADS1115_ADDRESS, &Wire);
  Wire.begin();
  ADS.begin();
  ADS.setGain(0);      // 6.144 volt
  ADS.setDataRate(7);  // fast
  ADS.setMode(0);      // continuous mode
  ADS.readADC(0);      // first read to trigger
}

static float getPressure() {
  // voltageZero = 0.5V --> 2666.7 (ADS 15 bit), voltageMax = 4.5V --> 24000
  // pressure gauge range 0-1.2MPa - 0-12 bar, 1 bar = 1777.8 counts
  return (ADS.getValue() - 2666) / 1777.8f;
}

bool readPressure(uint32_t now) {
  if ((now - last_pressure_read_time) > PRESSURE_READ_PERIOD_MS) {
    pressure_read = getPressure();
    pressure_smoothed = smoothPressure.updateEstimate(pressure_read);
    last_pressure_read_time = now;
    return true;
  }
  return false;
}

bool readTemperature(uint32_t now) {
  if ((now - last_temp_read_time) > TEMP_READ_PERIOD_MS) {
    double newReading = thermocouple.readCelsius();
    // The machine never runs near freezing or above 200 C; anything outside is a
    // sensor fault (the MAX6675 returns 0 or NaN with an open thermocouple). Keep
    // the last good value so the PID does not react to it.
    if (newReading > 1 && newReading < 200) {
      temperature_read = newReading;
      temperature_smoothed = smoothTemperature.updateEstimate(temperature_read);
    } else {
      temperatureFaults++;
    }
    last_temp_read_time = now;
    return true;
  }
  return false;
}
