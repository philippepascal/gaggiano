// include the library code:
#include <LiquidCrystal.h>
#include <math.h>

// named constant for the pin the sensor is connected to
const int sensorPin = A0;
// room temperature in Celsius
const float baselineTemp = 20.0;
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
int base = 100;

void setup() {
  // put your setup code here, to run once:
  lcd.begin(16, 2);
}

void loop() {
  // put your main code here, to run repeatedly:
  
int start = millis();

  // convert the voltage to temperature in degrees C
  // the sensor changes 10 mV per degree
  // the datasheet says there's a 500 mV offset
  // ((voltage - 500 mV) times 100)
  int sensorVal = analogRead(sensorPin);
  float voltage = (sensorVal / 1024.0) * 5.0;
  float temperature = (voltage - 0.5) * 100;

  lcd.setCursor(0,0);
  lcd.print("s:100 | t:"+String(temperature)+"C");

  lcd.setCursor(0,1);
  int r = random (-1,2);
  base = base + r;
  char randvalue[3] = "  ";
  char value[4] = "   ";
  sprintf(value, "%03d", base);
  sprintf(randvalue, "%02d", r);

  String s = ""+String(value)+" "+String(randvalue);

  int end = millis();

  s = s + " " + String(end-start);

  lcd.print(s);

  delay(300 - (end - start));
}
