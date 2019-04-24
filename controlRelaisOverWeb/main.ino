#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <Ticker.h>


const int switchPin = D1;
String ssid;
String wpakey;
String webpass;
bool reset = 0;
char trimKey = 0xD;

Ticker flipper;
Ticker serialInput;

String SerialInput = "";
char SerialChar;

ESP8266WebServer server(80);

void flip()
{
  int state = digitalRead(LED_BUILTIN);  // get the current state of GPIO1 pin
  digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state 
 //flipper.detach();
}

bool switchRelay()
{
  int state = digitalRead(switchPin);
  digitalWrite(switchPin, !state);
  return state;
}


//Check if header is present and correct
bool is_authentified(){
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")){
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}

// page, also called for disconnect
void handleLogin(){
  String msg;
  if (server.hasHeader("Cookie")){
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")){
    Serial.println("Disconnection");
    server.sendHeader("Location","/login");
    server.sendHeader("Cache-Control","no-cache");
    server.sendHeader("Set-Cookie","ESPSESSIONID=0");
    server.send(301);
    return;
  }
  if (server.hasArg("PASSWORD")){
    if (server.arg("PASSWORD") == webpass ){
      server.sendHeader("Location","/");
      server.sendHeader("Cache-Control","no-cache");
      server.sendHeader("Set-Cookie","ESPSESSIONID=1");
      server.send(301);
      Serial.println("Log in Successful");
      return;
    }
  msg = "Wrong username/password! try again.";
  Serial.println("Log in Failed");
  }
  String content = "<html><body><form action='/login' method='POST'>To log in, please use : admin/admin<br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  content += "You also can go <a href='/guest'>here</a></body></html>";
  server.send(200, "text/html", content);
}

void checkAuth()
{
   if (!is_authentified()){
    server.sendHeader("Location","/login");
    server.sendHeader("Cache-Control","no-cache");
    server.send(301);
    return;
  }
}

//root page can be accessed only if authentification is ok
void handleRoot(){
  Serial.println("Enter handleRoot");
  String header;
  checkAuth();

  String switchState = "off";
  
  if(switchRelay() == true)
  {
    switchState = "on";
  }

  String content = "<!DOCTYPE html><head><title>title</title>";
  content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>";
  content += "<style> html, body { width: 100%; height: 100%; margin: 0; padding: 0; background-color: white; }</style><body>";
  content += "<H2>State: " + switchState + "</H2><br>";
  content += "<br /><form action='/switch' target='_self' method='post'><input type='submit' value='Switch'></form>";
  content += "<br /><br />";
  content += "<br /><form action='/login?DISCONNECT=YES' target='_self' method='get'><input type='submit' value='LOGOUT'></form>"; 
  content += "</body></html>";
  
  server.send(200, "text/html", content);
}


//root page can be accessed only if authentification is ok
void handleSwitch(){
  Serial.println("Enter handleSwitch");
  String header;
  checkAuth();

  String switchState = "on";
  
  if(switchRelay() == true)
  {
    switchState = "off";
  }
  
  String content = "<!DOCTYPE html><head><title>title</title>";
  content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"/>";
  content += "<style> html, body { width: 100%; height: 100%; margin: 0; padding: 0; background-color: white; }</style><body>";
  content += "<H2>State: " + switchState + "</H2><br>";
  content += "<br /><form action='/switch' target='_self' method='post'><input type='submit' value='Switch'></form>";
  content += "<br /><br />";
  content += "<br /><form action='/login?DISCONNECT=YES' target='_self' method='get'><input type='submit' value='LOGOUT'></form>"; 
  content += "</body></html>";
  
  server.send(200, "text/html", content);
}

//no need authentification
void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}



void SerialInputLoop() {
  
  Serial.println(SerialInput);
  
  while(Serial.available()) {
      SerialChar = Serial.read();

      int c = static_cast<int>(SerialChar);
      Serial.println(c);
      
      if((SerialChar == 0xA) || (SerialChar == 0xD))
      {
        Serial.println(SerialInput);
        SerialInput = "";
        SerialChar = 0;
      }
      SerialInput.concat(SerialChar);
  }

  // IMPORTANT:
  // We must call 'yield' at a regular basis to pass
  // control to other tasks.

  //yield();

}


void setup(void){
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  flipper.attach(0.5, flip);
  
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("Boot in 3 sec...");  
  delay(1000);
  Serial.println("Boot in 2 sec...");  
  delay(1000);
  Serial.println("Boot in 1 sec...");  
  delay(1000);
  
  //Serial.setDebugOutput(true);
  
  String rx_str = "";
  char rx_byte = 0;
  
  if (Serial.available() > 0) {    // is a character available?
    Serial.println("Read Serial Input... ");
    int c = Serial.read();
    while(c >= 0)
    {
      rx_byte = static_cast<char>(c); //cast int to char
      Serial.println(c);
      rx_str += rx_byte;
      c = Serial.read();
    }
    
    if(rx_str == "reset")
    {
      reset = 1;
    }
  } // end: if (Serial.available() > 0)

  SPIFFS.begin();
  
  // init SPIFFS and Config
  if (!SPIFFS.exists("/formatComplete.txt") || reset) {
    flipper.attach(0.1, flip);
    Serial.println("Please wait for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");

    File f;
    
    f = SPIFFS.open("/formatComplete.txt", "w");
    if (!f) {
        Serial.println("file open failed, formatting failed.");
        f.close();
        exit(1);
    } else {
        f.print("Format Complete");
    }
    f.close();

    f = SPIFFS.open("/ssid.txt", "w");
    if (!f) {
        Serial.println("Could not write to SPIFF.");
        exit(1);
    } else {
        f.print("lampe");
    }
    f.close();

    f = SPIFFS.open("/wpakey.txt", "w");
    if (!f) {
        Serial.println("Could not write to SPIFF.");
        exit(1);
    } else {
        f.print("wpaPassWord");
    }
    f.close();

    f = SPIFFS.open("/webpass.txt", "w");
    if (!f) {
        Serial.println("Could not write to SPIFF.");
        exit(1);
    } else {
        f.print("loginPassWord");
    }
    f.close();

  } else {
    Serial.println("SPIFFS is formatted. Config initialized.");
  }

  flipper.attach(0.5, flip);
  
  Serial.println("Read config...");

  File f;
  
  f = SPIFFS.open("/webpass.txt", "r");
  if(!f)
  {
    Serial.println("Problem reading config.");
    exit(1);
  } else {
    webpass = f.readString();
  }
  f.close();
  
  f = SPIFFS.open("/ssid.txt", "r");
  if(!f)
  {
    Serial.println("Problem reading config.");
    exit(1);
  } else {
    ssid = f.readString();
    Serial.println("SSID:" + ssid);
  }
  f.close();

  f = SPIFFS.open("/wpakey.txt", "r");
  if(!f)
  {
    Serial.println("Problem reading config.");
    exit(1);
  } else {
    wpakey = f.readString();
  }
  f.close();
  
  pinMode(switchPin, OUTPUT);

  // create WLAN-AP
  const char * apid = ssid.c_str();
  const char * key = wpakey.c_str();
  
  WiFi.softAP(apid, key);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/", handleRoot);
  server.on("/switch", handleSwitch);
  server.on("/login", handleLogin);
  server.on("/guest", [](){
    server.send(200, "text/plain", "DeviceID:" + ssid);
  });

  server.onNotFound(handleNotFound);
  //here the list of headers to be recorded
  const char * headerkeys[] = {"User-Agent","Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize );
  server.begin();
  Serial.println("HTTP server started");

  flipper.attach(2, flip); // this flips the internal led
  serialInput.attach(1, SerialInputLoop);
}

void loop(void){
  server.handleClient();
}
