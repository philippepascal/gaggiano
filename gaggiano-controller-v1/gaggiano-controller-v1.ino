// --------- includes ------------------

#include <AutoPID.h>

// --------- global vars ----------
double temperature_read = 0;
double temperatureSetPoint = 0;
double boiler_relay_output;
double boiler_bb_range = 3;
double boiler_PID_cicle = 1000;
double boiler_PID_KP = 10;
double boiler_PID_KI = 0.2;
double boiler_PID_KD = 0.1;

double pressure_read = 0;
double pressureSetPoint = 0;
double pump_dimmer_output;
double pump_bb_range = 1;
double pump_PID_cicle = 100;
double pump_PID_KP = 1;
double pump_PID_KI = 0.1;
double pump_PID_KD = 0.05;

// AD1115 for pressure ------------------

#include <Wire.h>
#include <ADS1X15.h>
ADS1115 ADS;

// Solenoid Valve -----------------------

#define valvePin PC13

// Pump Pulse Skip Modulation -----------

#include "PSM.h"
// #define zcPin PA0
// #define dimmerPin PA1
#define zcPin PA15
#define dimmerPin PB3
#define ZC_MODE RISING
#define PUMP_RANGE 255
PSM *pump;

//pid settings and gains
#define PUMP_OUTPUT_MIN 0
#define PUMP_OUTPUT_MAX 255

// empirical values
#define PUMP_KP 5
#define PUMP_KI .1
#define PUMP_KD .05
//input/output variables passed by reference, so they are updated automatically
AutoPID pumpPID(&pressure_read, &pressureSetPoint, &pump_dimmer_output, PUMP_OUTPUT_MIN, PUMP_OUTPUT_MAX, PUMP_KP, PUMP_KI, PUMP_KD);

// boiler thermo couple -----------------

#include <max6675.h>
#define MAX6675_CS PA6
#define MAX6675_SO PB4
#define MAX6675_SCK PA5

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);

// ---------  boiler PID ---------------
//pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 100

// empirical values
#define KP 10
#define KI .2
#define KD .1

#define BOILER_RELAY_PIN PB5
uint32_t boiler_relay_pin_channel;  //timer channel for the boiler pin
HardwareTimer *MyTim;               //timer for the boiler pin

//input/output variables passed by reference, so they are updated automatically
AutoPID boilerPID(&temperature_read, &temperatureSetPoint, &boiler_relay_output, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

// HardwareSerial screenSerial(2);
HardwareSerial screenSerial(PA3, PA2);


// ---------------------------

void setup() {
  //Communications --------------
  Serial.begin(9600);
  delay(5000);
  Serial.println("serial works");

  screenSerial.begin(115200);  //default ports...
  screenSerial.println("hello screen");

  //Pressure reading ------------
  Wire.setSDA(PB7);  //should not be necessary.. default value
  Wire.setSCL(PB6);  //should not be necessary.. default value
  ADS = ADS1115(0x48, &Wire);

  Wire.begin();

  ADS.begin();
  ADS.setGain(0);      // 6.144 volt
  ADS.setDataRate(7);  // fast
  ADS.setMode(0);      // continuous mode
  ADS.readADC(0);      // first read to trigger

  // Pump
  //if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  pumpPID.setBangBang(pump_bb_range);
  //set PID update interval to 1000ms
  pumpPID.setTimeStep(pump_PID_cicle);

  //Solenoid --------------------
  pinMode(valvePin, OUTPUT);
  // just in case
  digitalWrite(valvePin, LOW);

  //Pump -------------------------
  pump = new PSM(zcPin, dimmerPin, PUMP_RANGE, ZC_MODE, 2);
  pump->set(0);

  //Boiler PID -------------------
  pinMode(BOILER_RELAY_PIN, OUTPUT);
  //if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  boilerPID.setBangBang(boiler_bb_range);
  //set PID update interval to 1000ms
  boilerPID.setTimeStep(boiler_PID_cicle);

  // Automatically retrieve TIM instance and channel associated to pin
  // This is used to be compatible with all STM32 series automatically.
  TIM_TypeDef *Instance = (TIM_TypeDef *)pinmap_peripheral(digitalPinToPinName(BOILER_RELAY_PIN), PinMap_PWM);
  boiler_relay_pin_channel = STM_PIN_CHANNEL(pinmap_function(digitalPinToPinName(BOILER_RELAY_PIN), PinMap_PWM));
  // Instantiate HardwareTimer object. Thanks to 'new' instantiation, HardwareTimer is not destructed when setup() function is finished.
  MyTim = new HardwareTimer(Instance);

  // Configure and start PWM
  // MyTim->setPWM(boiler_relay_pin_channel, pin, 5, 10, NULL, NULL); // No callback required, we can simplify the function call
  // MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, 5, 10);  // 5 Hertz, 10% dutycycle
  MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, 5, 0);
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.print(" counter: ");
  Serial.println(pump->getCounter());

  // --- read sensors ----
  temperature_read = thermocouple.readCelsius();
  pressure_read = getPressure();

  //----------------------
  readMessage();

  //Boiler PID -----------
  boilerPID.run();
  Serial.print(" boiler output: ");
  Serial.print(boiler_relay_output);
  MyTim->setPWM(boiler_relay_pin_channel, BOILER_RELAY_PIN, 5, boiler_relay_output);

  //Pump and Solenoid (coupled)
  pumpPID.run();
  updatePump();
  Serial.print(" pump output: ");
  Serial.print(pump_dimmer_output);
  Serial.print(" valve state: ");
  Serial.print(digitalRead(valvePin));

  //----------------------
  sendStatus();

  // ---------------------
  delay(500);  //200 is a decent value for screen updates
}

// Utilities --------------------

void updatePump() {
  if (pressureSetPoint > 0) {
    // open Solenoid
    digitalWrite(valvePin, HIGH);

    pump->set(pump_dimmer_output);
    // digitalWrite(dimmerPin,HIGH);
  } else {
    pump->set(0);
    // digitalWrite(dimmerPin,LOW);

    // close Solenoid
    digitalWrite(valvePin, LOW);
  }
}

void updateAdvancedSettings() {
  //if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  boilerPID.setBangBang(boiler_bb_range);
  //set PID update interval to 1000ms
  boilerPID.setTimeStep(boiler_PID_cicle);
  boilerPID.setGains(boiler_PID_KP, boiler_PID_KI, boiler_PID_KD);

  //if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  pumpPID.setBangBang(pump_bb_range);
  //set PID update interval to 1000ms
  pumpPID.setTimeStep(pump_PID_cicle);
  pumpPID.setGains(pump_PID_KP, pump_PID_KI, pump_PID_KD);
}

float getPressure() {  //returns sensor pressure data
                       // 5V/1024 = 1/204.8 (10 bit) or 6553.6 (15 bit)
                       // voltageZero = 0.5V --> 102.4(10 bit) or 3276.8 (15 bit)
                       // voltageMax = 4.5V --> 921.6 (10 bit) or 29491.2 (15 bit)
                       // range 921.6 - 102.4 = 819.2 or 26214.4
                       // pressure gauge range 0-1.2MPa - 0-12 bar
                       // 1 bar = 68.27 or 2184.5

  return ADS.getValue() / 1706.6f - 1.49f;
}

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

void readMessage() {
  char m[100] = "";
  if (screenSerial.available()) {
    Serial.println("received");
    strcat(m, screenSerial.readStringUntil('\n').c_str());
    Serial.println(m);
  }
  int messageSize = myIndexOF(m, '|', 0);
  if (messageSize > 0) {
    //message is complete..unpack
    int cursor = 0;
    int endCursor = myIndexOF(m, ';', cursor);
    int sender = -1;
    if (endCursor > 0 && endCursor < messageSize) {
      sender = atoi(mySubString(m, cursor, endCursor));
      cursor = endCursor + 1;
      endCursor = myIndexOF(m, ';', cursor);
    }
    if (sender == 1) {  //command from the screen
      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        temperatureSetPoint = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        pressureSetPoint = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }
    } else if (sender == 2) {  //advanced settings
      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        boiler_bb_range = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        boiler_PID_cicle = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        boiler_PID_KP = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        boiler_PID_KI = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        boiler_PID_KD = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        pump_bb_range = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        pump_PID_cicle = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        pump_PID_KP = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        pump_PID_KI = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      if (endCursor > 0 && endCursor < messageSize) {
        float value = atof(mySubString(m, cursor, endCursor));
        pump_PID_KD = value;
        cursor = endCursor + 1;
        endCursor = myIndexOF(m, ';', cursor);
      }

      updateAdvancedSettings();
    }
  }
}

void sendStatus() {
  char message[100] = "";
  sprintf(message, "0;%2f;%2f;%d;|", temperature_read, pressure_read, digitalRead(valvePin));
  Serial.print(" sent: ");
  Serial.println(message);
  screenSerial.println(message);
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
// trying this for setting PWM frequency for pin PB5 that we will use for boiler relay. hopefully it's timer is isolated from other functions
// https://github.com/stm32duino/STM32Examples/blob/main/examples/Peripherals/HardwareTimer/All-in-one_setPWM/All-in-one_setPWM.ino
//
// weird, but it looks like I had to connect the valve relay to normally closed...
//
// picked up PSM library from https://github.com/banoz/PSM.Library.git, cloned and copied manually
//