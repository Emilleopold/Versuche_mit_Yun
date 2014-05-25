
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

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.
YunServer server;

boolean Status = false;
char Orderstring[255];
char SysVarTemperatur[] = "Temperatur";
long Temperatur = 0;


void setup() {
  Serial.begin(9600);
  // Wait until a Serial Monitor is connected.
//  while (!Serial);
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
}

void loop() {
  // Get clients coming from server
  YunClient Yclient = server.accept();

  // There is a new client?
  if (Yclient) {
    // Process request
    process(Yclient);

    // Close connection and free resources.
    Yclient.stop();
  }

  delay(50); // Poll every 50ms

/*
  if (Status == false) {
    client.get("http://192.168.11.220:8181/do.exe?r1=dom.GetObject(\"Temperatur\").State(\"50\")");
    Status = true;
    Serial.println("50 gesendet");
    }
  else {
    client.get("http://192.168.11.220:8181/do.exe?r1=dom.GetObject(\"Temperatur\").State(\"10\")");
    Status = false;
    Serial.println("10 gesendet");
    }

*/  
  HttpClient Hclient;
  sprintf(Orderstring,"http://192.168.11.220:8181/do.exe?r1=dom.GetObject(\"%s\").State(\"%d\")",SysVarTemperatur, Temperatur);
  Serial.println(Orderstring);
  ++Temperatur;
  Hclient.get(Orderstring);

  delay(10*1000); // alle 10 Sekunden
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
