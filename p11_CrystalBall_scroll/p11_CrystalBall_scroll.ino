/*
  Arduino Starter Kit example
  Project 11 - Crystal Ball

  This sketch is written to accompany Project 11 in the Arduino Starter Kit

  Parts required:
  - 220 ohm resistor
  - 10 kilohm resistor
  - 10 kilohm potentiometer
  - 16x2 LCD screen
  - tilt switch

  created 13 Sep 2012
  by Scott Fitzgerald

  https://store.arduino.cc/genuino-starter-kit

  This example code is part of the public domain.
*/

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// set up a constant for the tilt switch pin
//const int switchPin = 6;

// variable to hold the value of the switch pin
//int switchState = 0;

// variable to hold previous value of the switch pin
//int prevSwitchState = 0;

// a variable to choose which reply from the crystal ball
int reply;
char buffer[] = "                ";
char printthis[16] = "                ";

void setup() {
  // set up the number of columns and rows on the LCD
  lcd.begin(16, 2);

  // set up the switch pin as an input
  //pinMode(switchPin, INPUT);

  // Print a message to the LCD.
  lcd.print("Ask the");
  // set the cursor to column 0, line 1
  // line 1 is the second row, since counting begins with 0
  lcd.setCursor(0, 1);
  // print to the second line
  lcd.print("Crystal Ball!");
}

void loop() {
  // check the status of the switch
  //switchState = digitalRead(switchPin);

  // compare the switchState to its previous state
  //if (switchState != prevSwitchState) {
    // if the state has changed from HIGH to LOW you know that the ball has been
    // tilted from one direction to the other
    //if (switchState == LOW) {
      // randomly chose a reply
      reply = random(8);
      // clean up the screen before printing a new reply
      lcd.clear();
      // set the cursor to column 0, line 0
      lcd.setCursor(0, 0);
      // print some text
      lcd.print("the ball says:");
      // move the cursor to the second line
      lcd.setCursor(0, 1);

      // choose a saying to print based on the value in reply
      switch (reply) {
        case 0:
          scroll("             Yes");
          break;

        case 1:
          scroll("     Most likely");
          break;

        case 2:
          scroll("       Certainly");
          break;

        case 3:
          scroll("    Outlook good");
          break;

        case 4:
          scroll(".         Unsure");
          break;

        case 5:
          scroll("       Ask again");
          break;

        case 6:
          scroll(".       Doubtful");
          break;

        case 7:
          scroll(".               No");
          break;
      }
  //  }
  //}
  // save the current switch state as the last state
  //prevSwitchState = switchState;
}

void scroll(char txt[]) {
  for(int i=15;i>=0;i--) {
    for(int k=15;k>=1;k--) {
      printthis[k] = printthis[k-1];
    }
    printthis[0] = txt[i];
    lcd.setCursor(0, 1);
    lcd.print(printthis); 
    delay(200);
  }
}