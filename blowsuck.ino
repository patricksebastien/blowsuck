//Libraries
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>


bool debugSerial = false;
const long period=5; //time between samples in milliseconds
#define LED_BUILTIN 19


// launch webserver and udp
WiFiUDP udp;
WebServer server(80);

// var / constants
bool error=0;
long startMillis=0;                       //start sampling milliseconds time
int i = 0;
int statusCode;
String st;
String content;
String esid;
String epass = "";
String eudpip;
IPAddress udpip;
String eudpport;
float press=0.00; 

//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);
bool readCFSensor(byte sensorAddress);

void setup() {
  btStop();
  if(debugSerial) {
    Serial.begin(9600);                     //begin serial port
  }
  Wire.begin();                           //begin i2c bus
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM

  /* 
  // CLEAR EEPROM to reset wifi credentials
  // AP will start (blowsuckAP), visit: http://192.168.4.1
  for (int i = 0 ; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }
  */

  delay(10);
  pinMode(0, INPUT);
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  
  for (int i = 96; i < 128; ++i)
  {
    eudpip += char(EEPROM.read(i));
  }
  for (int i = 128; i < 160; ++i)
  {
    eudpport += char(EEPROM.read(i));
  }

  if(debugSerial) {
    Serial.println();
    Serial.println("Reading EEPROM");
    Serial.print("SSID: ");
    Serial.println(esid);
    Serial.print("PASS: ");
    Serial.println(epass);
    Serial.print("UDP SERVER IP: ");
    Serial.println(eudpip);
    Serial.print("UDP SERVER PORT: ");
    Serial.println(eudpport);
  }

  udpip.fromString(eudpip);

  
  WiFi.begin(esid.c_str(), epass.c_str());
}

void loop() {

  if ((WiFi.status() == WL_CONNECTED)) {
      startMillis=millis();                                             //save the starting time
      error = readCFSensor(0x6D);              //start conversion and read on pressure sensor ad 0x6D address
      while((millis()-startMillis) < period);                           //waits until period done
  }

  if (testWifi() && (digitalRead(0) != 0)) {
    return;
  } else {
    if(debugSerial) {
      Serial.println("Connection Status Negative / D0 HIGH");
      Serial.println("Turning the HotSpot On");
    }
    launchWeb();
    setupAP();// Setup HotSpot
  }
  
  while ((WiFi.status() != WL_CONNECTED))
  {
    delay(100);
    server.handleClient();
  }
  
}





// XGZP6897D I2C
bool readCFSensor(byte sensorAddress) {

  byte reg0xA5=0;

  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0xA5);                         //send 0xA5 register address
  Wire.endTransmission();                   //send Stop
  Wire.requestFrom(sensorAddress,byte(1));  //send Start and read 1 byte command from sensor address
  if(Wire.available()){                     //check if data is available on i2c buffer
    reg0xA5 = Wire.read();                  //read 0xA5 register value
  }
  Wire.endTransmission();                   //send Stop


  reg0xA5 = reg0xA5 & 0xFD;                 //mask 0xA5 register AND 0xFD to set ADC output calibrated data
  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0xA5);                         //send 0xA5 register address
  Wire.write(reg0xA5);                      //send 0xA5 regiter new value
  Wire.endTransmission();                   //send Stop

  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0x30);                         //send 0x30 register address
  Wire.write(0x0A);                         //set and start (0X0A = temperature + pressure, 0x01 just pressure)
  Wire.endTransmission();                   //send Stop

  byte reg0x30 = 0x30;                      //declare byte variable for 0x30 register copy (0x08 initializing for while enter)
  while((reg0x30 & 0x08) > 0) {             //loop while bit 3 of 0x30 register copy is 1
    delay(1);                               //1mS delay
    Wire.beginTransmission(sensorAddress);  //send Start and sensor address
    Wire.write(0x30);                       //send 0x30 register address
    Wire.endTransmission();                 //send Stop
    Wire.requestFrom(sensorAddress,byte(1));//send Start and read 1 byte command from sensor address
    if(Wire.available()){                   //check if data is available on i2c buffer
      reg0x30 = Wire.read();                //read 0x30 register value
    }
    Wire.endTransmission();    //send Stop
  }

  unsigned long pressure24bit;              //declare 32bit variable for pressure ADC 24bit value
  byte pressHigh=0;                         //declare byte temporal pressure high byte variable
  byte pressMid=0;                          //declare byte temporal pressure middle byte variable
  byte pressLow=0;                          //declare byte temporal pressure low byte variable

  Wire.beginTransmission(sensorAddress);    //send Start and sensor address
  Wire.write(0x06);                         //send pressure high byte register address
  Wire.endTransmission();                   //send Stop
  Wire.requestFrom(sensorAddress,byte(3));  //send Start and read 1 byte command from sensor address
  
  while(Wire.available() < 3);              //wait for 3 byte on buffer
    pressHigh = Wire.read();                  //read pressure high byte
    pressMid = Wire.read();                   //read pressure middle byte
    pressLow = Wire.read();                   //read pressure low byte
  Wire.endTransmission();                   //send Stop

  pressure24bit = pressure24bit | pressHigh;
  pressure24bit = pressure24bit & 0x000000FF;
  pressure24bit = pressure24bit << 8;

  pressure24bit = pressure24bit | pressMid;
  pressure24bit = pressure24bit & 0x0000FFFF;
  pressure24bit = pressure24bit << 8;

  pressure24bit = pressure24bit | pressLow;
  pressure24bit = pressure24bit & 0x00FFFFFF;


  if(pressure24bit > 8388608) {                                         //check sign bit for two's complement
    press = (float(pressure24bit) - float(16777216)) * 0.0000078125;    //KPa negative pressure calculation
  }
  else {                                                                //no sign
    press = float(pressure24bit) * 0.0000078125;                        //KPa positive pressure calculation
  }

  //Send a packet
  udp.beginPacket(udpip, strtoul(eudpport.c_str(), NULL, 10));
  udp.print(String(press));
  udp.endPacket();
  
  if(debugSerial) {
    Serial.println(String(press));
  }
    
  return 0;
}



bool testWifi(void)
{
  int c = 0;
  //Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    if(debugSerial) {
      Serial.print("*");
    }
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}


void launchWeb()
{
  if(debugSerial) {
    Serial.println("");
  }
  if (WiFi.status() == WL_CONNECTED)
  
  if(debugSerial) {
    Serial.println("WiFi connected");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
  }
  createWebServer();
  // Start the server
  server.begin();
  if(debugSerial) {
    Serial.println("Server started");
  }
}


void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  if(debugSerial) {
    Serial.println("scan done");
  }
  if (n == 0)
    if(debugSerial) {
      Serial.println("no networks found");
    }
  else
  {
    if(debugSerial) {
      Serial.print(n);
      Serial.println(" networks found");
    }
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      if(debugSerial) {
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
      }
      //Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  if(debugSerial) {
    Serial.println("");
  }
  st = "<ol>";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "<li>";
    st += WiFi.SSID(i);
    st += " (";
    st += WiFi.RSSI(i);
    st += ")";
    //st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
    st += "</li>";
  }
  st += "</ol>";
  delay(100);
  WiFi.softAP("blowsuckAP", "");
  if(debugSerial) {
    Serial.println("Initializing_softap_for_wifi credentials_modification");
  }
  launchWeb();
  if(debugSerial) {
    Serial.println("over");
  }
}


void createWebServer()
{
  {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html><h1>Blow / Suck Wifi Credentials Update page</h1>";
      content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' required length=32><input name='pass' type='password' length=64><br /><label>UDP server IP / PORT:</label><input required placeholder='udp server ip' name='udpserverip' length=32><input placeholder='udp server port' name='udpserverport' required value='3333' length=4><br /><br /><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/scan", []() {
      //setupAP();
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>go back";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      String qudpserverip = server.arg("udpserverip");
      String qudpserverport = server.arg("udpserverport");

      // SID / PWD
      if (qsid.length() > 0 && qudpserverip.length() > 0 && qudpserverport.length() > 0) {
        if(debugSerial) {
          Serial.println("clearing eeprom");
        }
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        if(debugSerial) {
          Serial.println(qsid);
          Serial.println("");
          Serial.println(qpass);
          Serial.println("");
          Serial.println("writing eeprom ssid:");
        }
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          if(debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(qsid[i]);
          }
        }
        if(debugSerial) {
          Serial.println("writing eeprom pass:");
        }
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          if(debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(qpass[i]);
          }
        }
      
        // UDP SERVER
        if(debugSerial) {
          Serial.println("clearing eeprom");
        }
        for (int i = 96; i < 160; ++i) {
          EEPROM.write(i, 0);
        }
        if(debugSerial) {
          Serial.println(qudpserverip);
          Serial.println("");
          Serial.println(qudpserverport);
          Serial.println("");
          Serial.println("writing eeprom udp server:");
        }
        for (int i = 0; i < qudpserverip.length(); ++i)
        {
          EEPROM.write(96 + i, qudpserverip[i]);
          if(debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(qudpserverip[i]);
          }
        }
        if(debugSerial) {
          Serial.println("writing eeprom udp server port:");
        }
        for (int i = 0; i < qudpserverport.length(); ++i)
        {
          EEPROM.write(128 + i, qudpserverport[i]);
          if(debugSerial) {
            Serial.print("Wrote: ");
            Serial.println(qpass[i]);
          }
        }
        
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
        ESP.restart();
        
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        if(debugSerial) {
          Serial.println("Sending 404");
        }
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
    });
  }
}