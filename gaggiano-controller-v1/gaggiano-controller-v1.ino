// --------- includes ------------------

#include <AutoPID.h>

// AD1115 for pressure ------------------

#include <Wire.h>
#include <ADS1X15.h>
ADS1115 ADS;

// boiler thermo couple -----------------

#include <max6675.h>
#define MAX6675_CS PA6
#define MAX6675_SO PB4
#define MAX6675_SCK PA5

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);

// ---------  boiler PID ---------------
//pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 255

// empirical values to result in quarters
// of 255 values
#define KP 10
#define KI .2
#define KD .2

double temperature_read = 0;
double temperatureSetPoint = 0;
double boiler_relay_output;

double pressure_read = 0;
double pressureSetPoint = 0;
double pump_relay_output;

int solenoidState = 0;

//input/output variables passed by reference, so they are updated automatically
AutoPID myPID(&temperature_read, &temperatureSetPoint, &boiler_relay_output, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

// HardwareSerial screenSerial(2);
HardwareSerial screenSerial(PA3, PA2);


// ---------------------------
void setup() {
  // put your setup code here, to run once:
  Wire.setSDA(PB7);  //should not be necessary.. default value
  Wire.setSCL(PB6);  //should not be necessary.. default value
  ADS = ADS1115(0x48, &Wire);

  Wire.begin();

  ADS.begin();
  ADS.setGain(0);      // 6.144 volt
  ADS.setDataRate(7);  // fast
  ADS.setMode(0);      // continuous mode
  ADS.readADC(0);      // first read to trigger

  Serial.begin(9600);
  screenSerial.begin(115200);  //default ports...
  screenSerial.println("hello screen");
  // boiler PID
  myPID.setTimeStep(1000);
}

void loop() {
  // put your main code here, to run repeatedly:

  // --- read sensors ----
  temperature_read = thermocouple.readCelsius();
  pressure_read = getPressure();

  readMessage();
  sendStatus();

  // boiler PID
  myPID.run();
  //Serial.printf("The PID output is %f.\n",boiler_relay_output);
  delay(5000);  //200 is a decent value for screen updates
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
    if (endCursor > 0 && endCursor < messageSize) {
      int sender = atoi(mySubString(m, cursor, endCursor));
      if (sender != 1) {  //not comming from the controler
        //if it's 0, indicate loopback which means loss of connection with screen. should turn everything off
        return;
      }
      cursor = endCursor + 1;
      endCursor = myIndexOF(m, ';', cursor);
    }

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
  }
}

void sendStatus() {
  char message[100] = "";
  sprintf(message, "0;%2f;%2f;%d;|", temperature_read, pressure_read, ((pressureSetPoint > 0) ? 1 : 0));
  Serial.println("sent:");
  Serial.println(message);
  screenSerial.println(message);
}

// to enable serial on this board , you need to compile with CDC Serial....
// https://www.stm32duino.com/viewtopic.php?t=1353
//
// to upload with arduino, select DFU programmer in tools/upload method, then hold boot while pressing NRST once. board enters DFU mode. select DFU port in tools/port and click upload
// Wire library example
// https://github.com/stm32duino/Arduino_Core_STM32/blob/main/libraries/Wire/examples/i2c_scanner/i2c_scanner.ino
// https://www.stm32duino.com/viewtopic.php?t=1760