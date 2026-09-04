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
uint32_t pressureFaults = 0;
uint32_t i2cRecoveries = 0;
bool pressureStale = true;  // no good reading yet

static ADS1115 ADS;
static MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);
static SimpleKalmanFilter smoothPressure(0.6f, 0.6f, 0.1f);
static SimpleKalmanFilter smoothTemperature(0.25f, 0.25f, 0.01f);

static uint32_t last_temp_read_time = 0;
static uint32_t last_pressure_read_time = 0;
static uint32_t last_good_pressure_time = 0;
static uint32_t pressure_fault_streak = 0;  // consecutive failed reads

// Configures the ADC when it answers; when it does not (not wired, bus stuck) the
// next failed reads bring us back here through the recovery.
static void adsInit() {
  if (!ADS.begin()) {
    ADS.getError();
    return;
  }
  ADS.setGain(0);      // 6.144 volt
  ADS.setDataRate(7);  // fast
  ADS.setMode(0);      // continuous mode
  ADS.readADC(0);      // first read to trigger
  ADS.getError();      // clear whatever the init left behind
}

void sensorsSetup() {
  Wire.setSDA(I2C_SDA_PIN);  // should not be necessary.. default value
  Wire.setSCL(I2C_SCL_PIN);  // should not be necessary.. default value
  ADS = ADS1115(ADS1115_ADDRESS, &Wire);
  Wire.begin();
  adsInit();
}

// A slave interrupted mid-transfer (a glitch on the bus while it was driving SDA)
// keeps SDA low and every transaction after that times out; seen 2026-09-04, the
// ADS1115 stopped answering 11 s after a clean cycle and never came back. Clocking
// SCL by hand lets the slave finish its byte, a STOP releases the bus, then the
// peripheral and the ADC are set up again.
static void i2cBusRecover() {
  Wire.end();
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, OUTPUT_OPEN_DRAIN);
  for (int i = 0; i < 9; i++) {
    digitalWrite(I2C_SCL_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(5);
  }
  pinMode(I2C_SDA_PIN, OUTPUT_OPEN_DRAIN);  // STOP: SDA low then high while SCL is high
  digitalWrite(I2C_SDA_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(I2C_SDA_PIN, HIGH);
  delayMicroseconds(5);
  Wire.begin();
  adsInit();
  i2cRecoveries++;
}

static float toBar(int16_t raw) {
  // voltageZero = 0.5V --> 2666.7 (ADS 15 bit), voltageMax = 4.5V --> 24000
  // pressure gauge range 0-1.2MPa - 0-12 bar, 1 bar = 1777.8 counts
  return (raw - 2666) / 1777.8f;
}

// Returns true whenever an attempt was made, so the control law runs on a failed
// read too and can see `pressureStale`. While the bus is failing the attempts are
// spaced out (each one costs an I2C timeout) and the bus is recovered every
// I2C_RECOVER_AFTER_FAULTS attempts. The last good value is kept; the library
// would otherwise hand back 0, which reads as -1.5 bar.
bool readPressure(uint32_t now) {
  uint32_t period = pressure_fault_streak > 0 ? PRESSURE_RETRY_PERIOD_MS : PRESSURE_READ_PERIOD_MS;
  if ((now - last_pressure_read_time) <= period) return false;
  last_pressure_read_time = now;
  int16_t raw = ADS.getValue();
  if (ADS.getError() != ADS1X15_OK) {
    pressureFaults++;
    pressure_fault_streak++;
    pressureStale = (now - last_good_pressure_time) >= PRESSURE_STALE_MS;
    if (pressure_fault_streak % I2C_RECOVER_AFTER_FAULTS == 0) i2cBusRecover();
    return true;
  }
  pressure_fault_streak = 0;
  last_good_pressure_time = now;
  pressureStale = false;
  pressure_read = toBar(raw);
  pressure_smoothed = smoothPressure.updateEstimate(pressure_read);
  return true;
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
