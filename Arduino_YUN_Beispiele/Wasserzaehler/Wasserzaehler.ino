
// Possible commands are listed here:
//
// "digital/13"     -> digitalRead(13)
// "digital/13/1"   -> digitalWrite(13, HIGH)
// "analog/2/123"   -> analogWrite(2, 123)
// "analog/2"       -> analogRead(2)
// "mode/13/input"  -> pinMode(13, INPUT)
// "mode/13/output" -> pinMode(13, OUTPUT)
/*
When the REST password is turned off, you can use a browser with the following URL structure :
http://YUNms.local/arduino/digital/13 : calls digitalRead(13);
http://YUNms.local/arduino/digital/13/1 : calls digitalWrite(13,1);
http://YUNms.local/arduino/digital/13/0 : calls digitalWrite(13,0);
http://YUNms.local/arduino/analog/9/123 : analogWrite(9,123);
http://YUNms.local/arduino/analog/2 : analogRead(2);
http://YUNms.local/arduino/mode/13/input : pinMode(13, INPUT);
http://YUNms.local/arduino/mode/13/output : pinMode(13, OUTPUT);
You can use the curl command from the command line instead of a browser if you prefer.
*/

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <HttpClient.h>
#include <EEPROM.h>
#include "Timer.h"
#include <Console.h>

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

boolean Status = false;
char Orderstring[255];
char SysVarTemperatur[] = "Temperatur";
char SysVarWasserzaehler[] = "Wasserzaehler";
float Temperatur = 0.0;
unsigned long Wasserzaehler = 0;
unsigned long WasserzaehlerOld = 0;
unsigned long OldMilli = 0;

int ledgn1 = 8; // 1. Grüne LED
int ledrd1 = 9; // 1. Rote LED
int ledgn2 = 10; // 2. Grüne LED
int ledrd2 = 11; // 2. Rote LED
int ledState1s = LOW;             // ledState used to set the LED in 1 Sec timer
int ledStateLoop = LOW;             // ledState used to set the LED in loop()

int ain0 = 0;

Timer t;


void setup() {
  Serial.begin(9600);
  // Wait until a Serial Monitor is connected.
  //while (!Serial);
  Serial.println("Bridge Init starten");
  // Bridge startup
  pinMode(13,OUTPUT);
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
  Serial.println("Bridge Init fertig");

  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();
  
    // initialize the digital pin as an output.
  pinMode(ledgn1, OUTPUT);
  pinMode(ledrd1, OUTPUT);
  pinMode(ledgn2, OUTPUT);
  pinMode(ledrd2, OUTPUT);

  int tick1SekEvent = t.every(1000, doAll1Sek, (void*)2);
  Serial.print("1 second tick started id=");
  Serial.println(tick1SekEvent);

  int tick10SekEvent = t.every(10000, doAll10Sek, (void*)2);
  Serial.print("10 second tick started id=");
  Serial.println(tick10SekEvent);

  int tick1MinEvent = t.every(60000, doAll1Min, (void*)2);
  Serial.print("1 minute tick started id=");
  Serial.println(tick1MinEvent);

  int tick1HourEvent = t.every(3600000, doAll1Hour, (void*)2); // 1h = 3.600.000 mSek
  Serial.print("1 hour tick started id=");
  Serial.println(tick1HourEvent);

  int tick6HourEvent = t.every(21600000, doAll6Hour, (void*)2); // 6h = 21.600.000 mSek
  Serial.print("6 hour tick started id=");
  Serial.println(tick6HourEvent);

  Wasserzaehler = ReadCountEEProm(0);

  Console.begin(); 
}

void loop() {
  //Serial.println();
  //Serial.println("Hier startet loop()");
  //TimeStamp("Yclient");
  // Get clients coming from server
  YunClient Yclient = server.accept();
  //TimeStamp("accept");
  // There is a new client?
  if (Yclient) {
    // Process request
    process(Yclient);
    // Close connection and free resources.
    Yclient.stop();
  }
  //TimeStamp("Stop");
  delay(50); // Poll every 50ms
  t.update();
  if (ledStateLoop == LOW)
    ledStateLoop = HIGH;
  else
    ledStateLoop = LOW;
  digitalWrite(ledgn1, ledStateLoop);
}

void process(YunClient client) {
  // read the command
  String command = client.readStringUntil('/');

  // is "digital" command?
  if (command == "digital") {
    digitalCommand(client);
  }

  // is "analog" command?
  if (command == "analog") {
    analogCommand(client);
  }

  // is "mode" command?
  if (command == "mode") {
    modeCommand(client);
  }
}

void digitalCommand(YunClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
  } 
  else {
    value = digitalRead(pin);
  }

  // Send feedback to client
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to "));
  client.println(value);

  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void analogCommand(YunClient client) {
  int pin, value;

  // Read pin number
  pin = client.parseInt();

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/') {
    // Read value and execute command
    value = client.parseInt();
    analogWrite(pin, value);

    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" set to analog "));
    client.println(value);

    // Update datastore key with the current pin value
    String key = "D";
    key += pin;
    Bridge.put(key, String(value));
  }
  else {
    // Read analog pin
    value = analogRead(pin);

    // Send feedback to client
    client.print(F("Pin A"));
    client.print(pin);
    client.print(F(" reads analog "));
    client.println(value);

    // Update datastore key with the current pin value
    String key = "A";
    key += pin;
    Bridge.put(key, String(value));
  }
}

void modeCommand(YunClient client) {
  int pin;

  // Read pin number
  pin = client.parseInt();

  // If the next character is not a '/' we have a malformed URL
  if (client.read() != '/') {
    client.println(F("error"));
    return;
  }

  String mode = client.readStringUntil('\r');

  if (mode == "input") {
    pinMode(pin, INPUT);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" configured as INPUT!"));
    return;
  }

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" configured as OUTPUT!"));
    return;
  }

  client.print(F("error: invalid mode "));
  client.print(mode);
}

void FloatToString( float val, unsigned int precision, char* Dest){
  // prints val with number of decimal places determine by precision
  // NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
  // example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)
  // Gibt den String in der globalen char-array variablen Str zurück
  long frac;
  char Teil1[30] = "";
  char Teil2[10] = "";
  if(val >= 0)
    frac = (val - long(val)) * precision;
  else
    frac = (long(val)- val ) * precision;
  ltoa(long(val), Teil1, 10);
  ltoa(frac, Teil2, 10);
  //Serial.println("Teil1");
  //PrintChar(Teil1);
  //Serial.println("Teil2");
  //PrintChar(Teil2);
  strcat(Teil1, ".");
  strcat(Teil1, Teil2);
  strcpy(Dest, Teil1);
} 


void TimeStamp(char* Str) {
  unsigned long Milli = millis();
  Serial.print(Str);
  Serial.print(" Differenzzeit [Sek] : ");
  Serial.println(Milli - OldMilli);
  OldMilli = Milli;
}

long ReadCountEEProm(int BaseAddress){
  long Count = 0;
  int Shift = BaseAddress + 3;
  for (int i = BaseAddress; i < BaseAddress+4; i++) {
    Count = Count << 8;
    Count = Count | long(EEPROM.read(Shift));
    Shift--;
  }
  return Count;
}

void WriteCountEEProm(long Count, int BaseAddress){
  int ByteX;
  for (int i = BaseAddress; i < BaseAddress+4; i++){
    ByteX = int(Count & 0x000000FF);
    EEPROM.write(i, ByteX);
    Count = Count >> 8;
  }
}

void EEPromWrite(void){
  Serial.println("EEPROM geschrieben");
  WriteCountEEProm(Wasserzaehler , 0);
}

void doAll1Sek(void* context) {
  //int time = (int)context;
  //Serial.print(time);
  //Serial.print(" 1 second tick: millis()=");
  //Serial.println(millis());
  // set the LED with the ledState of the variable:
  if (ledState1s == LOW)
    ledState1s = HIGH;
  else
    ledState1s = LOW;
  digitalWrite(ledrd1, ledState1s);
  Serial.print("Poti :" );
  Serial.println(analogRead(ain0));
  Wasserzaehler += long(analogRead(ain0)); 
}

void doAll10Sek(void* context) {
  int time = (int)context;
  //Serial.print(time);
  //Serial.print(" 10 second tick: millis()=");
  //Serial.println(millis());
  HttpClient Hclient;
  TimeStamp("Hclient");
  //char Str1[20];
  //FloatToString(Temperatur, 100, Str1);
  //sprintf(Orderstring,"http://192.168.11.220:8181/do.exe?r1=dom.GetObject(\"%s\").State(\"%s\")",SysVarTemperatur, Str1);
  sprintf(Orderstring,"http://192.168.11.220:8181/do.exe?r1=dom.GetObject(\"%s\").State(\"%d\")",SysVarWasserzaehler, Wasserzaehler);
  Serial.println(Orderstring);
  Console.println(Orderstring);
  //Serial.println(Temperatur);
  //Temperatur = Temperatur + 0.12;
  TimeStamp("Temp");
  Hclient.get(Orderstring);
  TimeStamp("Orderstring");
}

void doAll1Min(void* context) {
  int time = (int)context;
  //Serial.print(time);
  //Serial.print(" 1 minute tick: millis()=");
  //Serial.println(millis());
}

void doAll1Hour(void* context) {
  int time = (int)context;
  Serial.print(time);
  Serial.print(" 1 hour tick: millis()=");
  Serial.println(millis());
  if (Wasserzaehler != WasserzaehlerOld) {
    EEPromWrite();
    WasserzaehlerOld = Wasserzaehler;
  }
}

void doAll6Hour(void* context) {
  int time = (int)context;
  Serial.print(time);
  Serial.print(" 6 hour tick: millis()=");
  Serial.println(millis());
}

