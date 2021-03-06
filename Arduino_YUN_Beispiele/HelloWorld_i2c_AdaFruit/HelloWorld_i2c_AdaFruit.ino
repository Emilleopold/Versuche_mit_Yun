/*
 Demonstration sketch for Adafruit i2c/SPI LCD backpack
 using MCP23008 I2C expander
 ( http://www.ladyada.net/products/i2cspilcdbackpack/index.html )

 This sketch prints "Hello World!" to the LCD
 and shows the time.
 
  The circuit:
 * 5V to Arduino 5V pin
 * GND to Arduino GND pin
 * CLK to Analog #5 at YUN #3
 * DAT to Analog #4 at YUN #2 
*/

// include the library code:
#include "Wire.h"
#include "LiquidCrystal.h"

// Connect via i2c, default address #0 (A0-A2 not jumpered)
LiquidCrystal lcd(0);

int ledrd = 13; // Rote LED on board
int ledStateLoop = LOW;             // ledState used to set the LED in loop()


void setup() {
  // set up the LCD's number of rows and columns: 
  lcd.begin(20, 4);
  // Print a message to the LCD.
  lcd.print("hello, world!");
  
  pinMode(ledrd,OUTPUT);
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis()/1000);

  lcd.setBacklight(HIGH);
  delay(500);
  lcd.setBacklight(LOW);
  delay(500);
  
  if (ledStateLoop == LOW)
    ledStateLoop = HIGH;
  else
    ledStateLoop = LOW;
  digitalWrite(ledrd, ledStateLoop);

}

