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
double setPoint = 10;
double boiler_relay_output;

//input/output variables passed by reference, so they are updated automatically
AutoPID myPID(&temperature_read, &setPoint, &boiler_relay_output, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

// HardwareSerial screenSerial(2);
HardwareSerial screenSerial( PA3 , PA2 );


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

  String receivedMessage = "";
  while (screenSerial.available()) {
    char incomingChar = screenSerial.read();  // Read each character from the buffer
    if (incomingChar == '\n') {               // Check if the user pressed Enter (new line character)
                                              // Print the message
      Serial.print("screen sent: ");
      Serial.println(receivedMessage);

      // Clear the message buffer for the next input
      receivedMessage = "";
    } else {
      // Append the character to the message string
      receivedMessage += incomingChar;
    }
  }
  Serial.println("sending to screen");
  screenSerial.println("hello screen");

  // boiler PID
  myPID.run();
  //Serial.printf("The PID output is %f.\n",boiler_relay_output);
  delay(5000);
}

// to upload with arduino, select DFU programmer in tools/upload method, then hold boot while pressing NRST once. board enters DFU mode. select DFU port in tools/port and click upload