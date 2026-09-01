// include the library code:

// --------- Thermo coupler -----
/*    Max6675 Module  ==>   Arduino
 *    CS              ==>     D9
 *    SO              ==>     D10
 *    SCK             ==>     D8
 *    Vcc             ==>     Vcc (5v)
 *    Gnd             ==>     Gnd      */

#include <max6675.h>
#define MAX6675_CS   9
#define MAX6675_SO   10
#define MAX6675_SCK  8

MAX6675 thermocouple(MAX6675_SCK, MAX6675_CS, MAX6675_SO);
double temperature_read = 0.0;

// --------- Boiler Relay -----
// #define BOILER_RELAY_PIN A1
// lower timer2 in setup in order to 
// have a 30Hz PWM
#define BOILER_RELAY_PIN 3

// ------- temp setting ----
// use a 10k potentiometer to read a setting value
#define TEMP_SET_PIN A0

// --------- PID ---------------
#include <AutoPID.h>
//pid settings and gains
#define OUTPUT_MIN 0
#define OUTPUT_MAX 255

// default value
// #define KP .12
// #define KI .0003
// #define KD 0

// empirical values to result in quarters 
// of 255 values
#define KP 10
#define KI .2
#define KD .2

double setPoint = 0;
double boiler_relay_output;

//input/output variables passed by reference, so they are updated automatically
AutoPID myPID(&temperature_read, &setPoint, &boiler_relay_output, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

//-------- LCD --------------
#include <LiquidCrystal.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 7, 6, 5, 4);

// --------------------------

void setup() {
  // lower frequency of PWM on Pin 3 and 11 to 30Hz
  TCCR2A = _BV(COM0A1) | _BV(COM0B1) | _BV(WGM00); 
  // TCCR2B = _BV(CS00);
  TCCR2B = TCCR2B & B11111000 | B00000111; 

  // initialize LCD
  lcd.begin(16, 2);

  // ---------- PID -------------
  pinMode(BOILER_RELAY_PIN,OUTPUT);

  //if temperature is more than 10 degrees below or above setpoint, OUTPUT will be set to min or max respectively
  myPID.setBangBang(10);
  //set PID update interval to 1000ms
  myPID.setTimeStep(1000);
}

void loop() {
  // --- read temp ----
  temperature_read = thermocouple.readCelsius();
  // --- get new setpoint in case it changed ----
  double r = (double)analogRead(TEMP_SET_PIN); //macx 1024/150 
  setPoint = 150*(r/1024);
  // --- update PID calculations ---
  myPID.run(); //call every loop, updates automatically at certain time interval
  // --- set boiler output ---
  analogWrite(BOILER_RELAY_PIN, boiler_relay_output);
  // --- update screen ----
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("trd:"+String((int)temperature_read)+" tset:"+(int)setPoint);
  lcd.setCursor(0,1);
  lcd.print("op:"+String((int)boiler_relay_output));
  delay(500);
}

// https://ryand.io/AutoPID/
// https://randomnerdtutorials.com/arduino-k-type-thermocouple-max6675/

// boiler relay is zero cross trigger with <10ms delay
// 10ms means maximum frequency that it can respond to is 1/0.01 = 100hz
// zero cross trigger means that maximum theoretical frequency is 120hz with sync'd signal. would probably not work with very low values of output
// arduino supports a lower bound of 31Hz by setting timer 2 (pin 3 and 11)
// in addition set PID settings to result in outputs close to quarters of 255, as smaller values have random effect due to relay being zero cross
//
