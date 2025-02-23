// --------- includes ------------------

#include <AutoPID.h>

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
HardwareSerial screenSerial( PA3, PA2 );


// ---------------------------
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  screenSerial.begin(115200);  //default ports...
  screenSerial.println("hello screen");
  // boiler PID
  myPID.setTimeStep(1000);
}

void loop() {
  // put your main code here, to run repeatedly:

  readMessage();
  sendStatus();

  char m[100];
  sprintf(m,"status: temp %2f; pressure %2f",temperatureSetPoint,pressureSetPoint);
  Serial.println(m);

  // boiler PID
  myPID.run();
  //Serial.printf("The PID output is %f.\n",boiler_relay_output);
  delay(500);
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
    Serial.println("something received");
    strcat(m, screenSerial.readStringUntil('\n').c_str());
    Serial.println(m);
  }
  int messageSize = myIndexOF(m, '|', 0);
  if (messageSize > 0) {
    //message is complete..unpack
    Serial.println("something complete needs parsing");
    int cursor = 0;
    int endCursor = myIndexOF(m, ';', cursor);
    if (endCursor > 0 && endCursor < messageSize) {
      int sender = atoi(mySubString(m, cursor, endCursor));
      Serial.println(sender);
      if (sender != 1) {  //not comming from the controler
        Serial.println("not from screen, ignore");
        return;
      }
      Serial.println("it's a message from controller");
      cursor = endCursor + 1;
      endCursor = myIndexOF(m, ';', cursor);
    }

    if (endCursor > 0 && endCursor < messageSize) {
      float value = atof(mySubString(m, cursor, endCursor));
      temperatureSetPoint = value;
      Serial.println(value);
      cursor = endCursor + 1;
      endCursor = myIndexOF(m, ';', cursor);
    }

    if (endCursor > 0 && endCursor < messageSize) {
      float value = atof(mySubString(m, cursor, endCursor));
      pressureSetPoint = value;
      Serial.println(value);
      cursor = endCursor + 1;
      endCursor = myIndexOF(m, ';', cursor);
    }
  }
}

void sendStatus() {
  char message[100] = "";
  sprintf(message, "0;%f;%f;%d;|", (float)(temperatureSetPoint-5), (float)(pressureSetPoint-1),((pressureSetPoint>0) ? 1 : 0));
  screenSerial.println(message);
}
// to upload with arduino, select DFU programmer in tools/upload method, then hold boot while pressing NRST once. board enters DFU mode. select DFU port in tools/port and click upload