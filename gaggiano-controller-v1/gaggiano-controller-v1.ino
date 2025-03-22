// --------- includes ------------------

#include <AutoPID.h>
#include <SimpleKalmanFilter.h>

// --------- global vars ----------

#define LOOP_PERIOD 10

// readings and outputs
double temperature_read = 0;
double temperature_smoothed = 0;
double boiler_relay_output;

double pressure_read = 0;
double pressure_smoothed = 0;
double pump_dimmer_output;
double pump_dimmer_output2;

// timers
uint32_t last_temp_read_time = 0;
uint32_t last_pressure_read_time = 0;
uint32_t last_read_message_time = 0;
uint32_t last_sent_message_time = 0;
uint32_t loopCounter = 0;

// inputs
double temperatureSetPoint = 0;
double boiler_bb_range = 3;
double boiler_PID_cycle = 1000;
double boiler_PID_KP = 10;
double boiler_PID_KI = 0.2;
double boiler_PID_KD = 0.1;

double pressureSetPoint = 0;
double pressureOutputPercent = 0;
double pump_bb_range = 1;
double pump_PID_cycle = 100;
double pump_PID_KP = 1;
double pump_PID_KI = 0.1;
double pump_PID_KD = 0.05;

// AD1115 for pressure ------------------

#include <Wire.h>
#include <ADS1X15.h>
ADS1115 ADS;

#define PRESSURE_READ_PERIOD 10

SimpleKalmanFilter smoothPressure(0.6f, 0.6f, 0.1f);
SimpleKalmanFilter smoothTemperature(0.25f, 0.25f, 0.01f);

// Solenoid Valve -----------------------

#define valvePin PC13

// Pump Pulse Skip Modulation -----------

#include "PSM.h"
#define zcPin PA15
#define dimmerPin PB3
#define ZC_MODE RISING
#define PUMP_RANGE 127
#define PUMP_LOW_MODE 30
PSM *pump;

// // pid settings and gains
// #define PUMP_OUTPUT_MIN 0
// #define PUMP_OUTPUT_MAX 127

// // empirical values
// #define PUMP_KP 5
// #define PUMP_KI .1
// #define PUMP_KD .05
// input/output variables passed by reference, so they are updated automatically
// AutoPID pumpPID(&pressure_read, &pressureSetPoint, &pump_dimmer_output, PUMP_OUTPUT_MIN, PUMP_OUTPUT_MAX, PUMP_KP, PUMP_KI, PUMP_KD);

// boiler thermo couple -----------------

#include <max6675.h>
#define MAX6675_CS PA6
#define MAX6675_SO PB4
#define MAX6675_SCK PA5

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);

#define TEMP_READ_PERIOD 250

// ---------  boiler PID ---------------
// pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 100

// empirical values
#define KP 10
#define KI .2
#define KD .1

#define BOILER_RELAY_FREQ 30

#define BOILER_RELAY_PIN PB5
uint32_t boiler_relay_pin_channel;  // timer channel for the boiler pin
HardwareTimer *MyTim;               // timer for the boiler pin

// input/output variables passed by reference, so they are updated automatically
// AutoPID boilerPID(&temperature_read, &temperatureSetPoint, &boiler_relay_output, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);
AutoPID boilerPID(&temperature_smoothed, &temperatureSetPoint, &boiler_relay_output, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

// ----------- messaging ---------------

// HardwareSerial screenSerial(2);
HardwareSerial screenSerial(PA3, PA2);

#define MESSAGE_READ_PERIOD 200
#define MESSAGE_SEND_PERIOD 200  // 200 is a decent value for screen updates; last known working value was 500

// ---------------------------

void setup() {
  // Communications --------------
  Serial.begin(9600);
  delay(1000);
  Serial.println("serial works");

  screenSerial.begin(115200);  // default ports...
  screenSerial.println("hello screen");

  // Pressure reading ------------
  Wire.setSDA(PB7);  // should not be necessary.. default value
  Wire.setSCL(PB6);  // should not be necessary.. default value
  ADS = ADS1115(0x48, &Wire);

  Wire.begin();

  ADS.begin();
  ADS.setGain(0);      // 6.144 volt
  ADS.setDataRate(7);  // fast
  ADS.setMode(0);      // continuous mode
  ADS.readADC(0);      // first read to trigger

  // Pump replaced with more "manual" control
  // if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  // pumpPID.setBangBang(pump_bb_range);
  // set PID update interval to 1000ms
  // pumpPID.setTimeStep(pump_PID_cycle);

  // Solenoid --------------------
  pinMode(valvePin, OUTPUT);
  // just in case
  digitalWrite(valvePin, LOW);

  // Pump -------------------------
  pump = new PSM(zcPin, dimmerPin, PUMP_RANGE, ZC_MODE, 2);
  pump->set(0);

  // Boiler PID -------------------
  pinMode(BOILER_RELAY_PIN, OUTPUT);
  // if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  boilerPID.setBangBang(boiler_bb_range);
  // set PID update interval to 1000ms
  boilerPID.setTimeStep(boiler_PID_cycle);

  // Automatically retrieve TIM instance and channel associated to pin
  // This is used to be compatible with all STM32 series automatically.
  TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(BOILER_RELAY_PIN), PinMap_PWM);
  boiler_relay_pin_channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(BOILER_RELAY_PIN), PinMap_PWM));
  // Instantiate HardwareTimer object. Thanks to 'new' instantiation, HardwareTimer is not destructed when setup() function is finished.
  MyTim = new HardwareTimer(Instance);

  // Configure and start PWM
  // MyTim->setPWM(boiler_relay_pin_channel, pin, 5, 10, NULL, NULL); // No callback required, we can simplify the function call
  // MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, 5, 10);  // 5 Hertz, 10% dutycycle
  MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, BOILER_RELAY_FREQ, 0);
}

void loop() {
  // put your main code here, to run repeatedly:

  uint32_t loopStart = millis();

  // --- read sensors ----
  bool tempUpdated = readTemperature(loopStart);
  bool pressuerUpdated = readPressure(loopStart);

  //----------------------
  readMessage(loopStart);

  // Boiler PID -----------
  if (tempUpdated) updateBoiler();

  // Pump and Solenoid (coupled)
  if (pressuerUpdated) updatePump();

  //----------------------
  sendStatus(loopStart);

  // ---------------------
  loopCounter++;
  uint32_t waitTime = LOOP_PERIOD - (millis() - loopStart);
  delay(waitTime);
}

// Utilities --------------------


bool readPressure(uint32_t now) {
  if ((now - last_pressure_read_time) > PRESSURE_READ_PERIOD) {
    // float elapsedTimeSec = elapsedTime / 1000.f;
    pressure_read = getPressure();
    // double previousSmoothedPressure = pressure_smoothed;
    pressure_smoothed = smoothPressure.updateEstimate(pressure_read);
    // double ressureChangeSpeed = (currentState.smoothedPressure - previousSmoothedPressure) / elapsedTimeSec;
    last_pressure_read_time = now;
    return true;
  }
  return false;
}

float getPressure() {
  // returns sensor pressure data
  //  5V/1024 = 1/204.8 (10 bit) or 6553.6 (15 bit)
  //  voltageZero = 0.5V --> 102.4(10 bit) or 3276.8 (15 bit)
  //  voltageMax = 4.5V --> 921.6 (10 bit) or 29491.2 (15 bit)
  //  range 921.6 - 102.4 = 819.2 or 26214.4
  //  pressure gauge range 0-1.2MPa - 0-12 bar
  //  1 bar = 68.27 or 2184.5

  // return ADS.getValue() / 1706.6f - 1.49f;

  // voltageZero = 0.5V --> 25.6 (8 bit) or 102.4 (10 bit) or 2666.7 (ADS 15 bit)
  // voltageMax = 4.5V --> 230.4 (8 bit) or 921.6 (10 bit) or 24000 (ADS 15 bit)
  // range 921.6 - 102.4 = 204.8 or 819.2 or 21333.3
  // pressure gauge range 0-1.2MPa - 0-12 bar
  // 1 bar = 17.1 or 68.27 or 1777.8
  return (ADS.getValue() - 2666) / 1777.8f;  // 16bit
}

bool readTemperature(uint32_t now) {
  if ((now - last_temp_read_time) > TEMP_READ_PERIOD) {
    double newReading = thermocouple.readCelsius();
    if (newReading > 1 || newReading < 200)  // not meant to run at freezing or too hot temperatures, so this should never be 0, just skip this reading.
    {
      temperature_read = newReading;
      temperature_smoothed = smoothTemperature.updateEstimate(temperature_read);
    }
    last_temp_read_time = now;
    return true;
  }
  return false;
}

void updatePump() {
  // pumpPID.run(); // currently unused

  double pumpValue;
  if (pressureOutputPercent > 0) {
    if (pressure_smoothed > pressureSetPoint) {
      pumpValue = 0;
    } else {
      int p = pressureOutputPercent;
      if (pressureOutputPercent > 20) {  // just safety, solenoid is closed!
        p = 20;
      }
      pumpValue = (p / 100) * PUMP_RANGE;
    }

    pump_dimmer_output2 = pumpValue;
    pump->set(pump_dimmer_output2);
    
  } else if (pressureSetPoint > 0) {
    // open Solenoid
    digitalWrite(valvePin, HIGH);

    double pumpValue;

    if (pressure_smoothed > pressureSetPoint) {
      pumpValue = 0;
    } else {
      float diff = pressureSetPoint - pressure_smoothed;
      pumpValue = PUMP_RANGE / (1.f + exp(1.7f - diff / 0.9f));
      if ((pumpValue - pump_dimmer_output2) > 1) {
        pumpValue = pump_dimmer_output2 + 0.2;
      }
    }

    pump_dimmer_output2 = pumpValue;
    pump->set(pump_dimmer_output2);

  } else {
    pump_dimmer_output2 = 0;
    pump->set(0);

    // close Solenoid
    digitalWrite(valvePin, LOW);
  }
}

void updateBoiler() {
  boilerPID.run();
  MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, BOILER_RELAY_FREQ, boiler_relay_output);
  // MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, BOILER_RELAY_FREQ, 50); // for testing frequency
}

// message handling ----------------------

int myIndexOF(const char *str, const char ch, int fromIndex) {
  const char *result = strchr(str + fromIndex, ch);
  if (result == NULL) {
    return -1;  // Substring not found
  } else {
    return result - str;  // Calculate the index
  }
}
char *mySubString(const char *str, int start, int end) {
  char *sub = (char *)malloc(sizeof(char) * (end - start));
  if (sub == NULL) {
    return NULL;
  }

  strncpy(sub, str + start, (end - start));
  sub[(end - start)] = '\0';

  return sub;
}

bool readMessage(uint32_t now) {
  if ((now - last_read_message_time) > MESSAGE_READ_PERIOD) {
    parseMessage();
    last_read_message_time = now;
    return true;
  }
  return false;
}

int getNextFloat(double *variable, char *message, int messageSize, int cursor) {
  int endCursor = myIndexOF(message, ';', cursor);
  if (endCursor > 0 && endCursor < messageSize) {
    *variable = atof(mySubString(message, cursor, endCursor));
    return endCursor + 1;
  } else {
    return -1;
  }
}

void parseMessage() {
  char m[500] = "";
  if (screenSerial.available()) {
    Serial.println("received");
    strcat(m, screenSerial.readStringUntil('\n').c_str());
    Serial.println(m);
  }
  int messageSize = myIndexOF(m, '|', 0);
  if (messageSize > 0) {
    // message is complete..unpack
    int cursor = 0;
    int endCursor = myIndexOF(m, ';', cursor);
    int sender = -1;
    if (endCursor > 0 && endCursor < messageSize) {
      sender = atoi(mySubString(m, cursor, endCursor));
      cursor = endCursor + 1;
    }
    if (sender == 1) {  // simple brew command from the screen
      cursor = getNextFloat(&temperatureSetPoint, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pressureSetPoint, m, messageSize, cursor);
      pressureOutputPercent = 0;
      Serial.print("temperatureSetPoint ");
      Serial.print(temperatureSetPoint);
      Serial.print(" pressureSetPoint ");
      Serial.println(pressureSetPoint);
    } else if (sender == 2) {  // special steam command (not by pressure, but by pump output)
      cursor = getNextFloat(&temperatureSetPoint, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pressureSetPoint, m, messageSize, cursor);  //max pressure
      if (cursor > 0) cursor = getNextFloat(&pressureOutputPercent, m, messageSize, cursor);
    } else if (sender == 9) {  // advanced settings
      cursor = getNextFloat(&boiler_bb_range, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&boiler_PID_cycle, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&boiler_PID_KP, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&boiler_PID_KI, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&boiler_PID_KD, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pump_bb_range, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pump_PID_cycle, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pump_PID_KP, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pump_PID_KI, m, messageSize, cursor);
      if (cursor > 0) cursor = getNextFloat(&pump_PID_KD, m, messageSize, cursor);

      updateAdvancedSettings();
    }
  }
}

void updateAdvancedSettings() {
  Serial.print(" advanced settings: ");
  Serial.print("boiler_bb_range:");
  Serial.print(boiler_bb_range);
  Serial.print("boiler_PID_cicle:");
  Serial.print(boiler_PID_cycle);
  Serial.print("boiler_PID_KP:");
  Serial.print(boiler_PID_KP);
  Serial.print("boiler_PID_KI:");
  Serial.print(boiler_PID_KI);
  Serial.print("boiler_PID_KD:");
  Serial.print(boiler_PID_KD);
  Serial.print("pump_bb_range:");
  Serial.print(pump_bb_range);
  Serial.print("pump_PID_cicle:");
  Serial.print(pump_PID_cycle);
  Serial.print("pump_PID_KP:");
  Serial.print(pump_PID_KP);
  Serial.print("pump_PID_KI:");
  Serial.print(pump_PID_KI);
  Serial.print("pump_PID_KD:");
  Serial.println(pump_PID_KD);

  // if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  boilerPID.setBangBang(boiler_bb_range);
  // set PID update interval to 1000ms
  boilerPID.setTimeStep(boiler_PID_cycle);
  boilerPID.setGains(boiler_PID_KP, boiler_PID_KI, boiler_PID_KD);

  // if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  // pumpPID.setBangBang(pump_bb_range);
  // set PID update interval to 1000ms
  // pumpPID.setTimeStep(pump_PID_cycle);
  // pumpPID.setGains(pump_PID_KP, pump_PID_KI, pump_PID_KD);
}

bool sendStatus(uint32_t now) {
  if ((now - last_sent_message_time) > MESSAGE_SEND_PERIOD) {
    char message[100] = "";
    sprintf(message, "0;%.2f;%.2f;%d;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%.2f;%d;|",
            // temperature_read,
            temperature_smoothed,
            pressure_smoothed,
            digitalRead(valvePin),
            boiler_relay_output,
            pump_dimmer_output2,
            temperatureSetPoint,
            boiler_bb_range,
            boiler_PID_cycle,
            boiler_PID_KP,
            boiler_PID_KI,
            boiler_PID_KD,
            loopCounter);
    Serial.print(" sent: ");
    Serial.println(message);
    screenSerial.println(message);
    last_sent_message_time = now;
    return true;
  }
  return false;
}

// Debugging Stuff -----------------------
void scanI2C() {

  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 255; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.

    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    } else {
      // Serial.print("error");
      // Serial.println(error);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found");
  else
    Serial.println("done");
}

// to enable serial on this board , you need to compile with CDC Serial....
// https://www.stm32duino.com/viewtopic.php?t=1353
//
// to upload with arduino, select DFU programmer in tools/upload method, then hold boot while pressing NRST once. board enters DFU mode. select DFU port in tools/port and click upload
// Wire library example
// https://github.com/stm32duino/Arduino_Core_STM32/blob/main/libraries/Wire/examples/i2c_scanner/i2c_scanner.ino
// https://www.stm32duino.com/viewtopic.php?t=1760
//
// also looks like relay on solenoid causes DFU not to work well when trying. set brew on before switching to DFU helps
//
// trying this for setting PWM frequency for pin PB5 that we will use for boiler relay. hopefully it's timer is isolated from other functions
// https://github.com/stm32duino/STM32Examples/blob/main/examples/Peripherals/HardwareTimer/All-in-one_setPWM/All-in-one_setPWM.ino
//
// weird, but it looks like I had to connect the valve relay to normally closed...
//
// picked up PSM library from https://github.com/banoz/PSM.Library.git, cloned and copied manually
//