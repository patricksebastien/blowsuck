// HARDWARE REQUIREMENTS
// ==================
// Example for ESP32 sending OSC midi to TouchOSC -> Virtual Midi -> DAW

// REQUIRED LIBRARIES
// ==================
// WiFiConnect (lite) library and required WifiClient and HTTPClient
// https://github.com/mrfaptastic/WiFiConnectLite


// REQUIRED CONFIGURATION
// ======================
// Change sendPort to target port
// Change sendIp to ip of target device

#include <Arduino.h>
#include <Wire.h>

// Load Wi-Fi library
#include <WiFi.h>

#include "WiFiConnect.h"
WiFiConnect wifiConnect;

#include <WiFiUdp.h>
WiFiUDP udp;

#include "microsmooth.h"
uint16_t *ptr;


IPAddress broadcastIp(255, 255, 255, 255); // <- slow?
IPAddress sendIp(192, 168, 0, 169); // <- change me
unsigned int sendPort = 12101; // <- touchosc port

bool debugSerial = false;
const long period=5; //time between samples in milliseconds
#define LED_BUILTIN 19

long startMillis=0;
uint8_t ccmap = 0;
uint8_t cclast = 0;
uint8_t ccmapunfilter = 0;
bool error=0;
float press=0.00;
float pressmap=0.00;




#include <MicroOscUdp.h>
// The number 1024 between the < > below  is the maximum number of bytes reserved for incomming messages.
// Outgoing messages are written directly to the output and do not need more reserved bytes.
MicroOscUdp<1024> oscUdp(&udp, sendIp, sendPort);


//Function Decalration
bool readCFSensor(byte sensorAddress);



/* We could also use button etc. to trigger the portal on demand within main loop */
void startWiFi() {

  //wifiConnect.resetSettings(); //helper to remove the stored wifi connection, comment out after first upload and re upload

  // Wifi Dies? Start Portal Again
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("DEBUG Trying to autoconnect...");
    // try to connect to wifi
    if (wifiConnect.autoConnect()) {
      Serial.println("DEBUG Autoconnected successfully!");
    } else {
      Serial.println("DEBUG Could not autoconnect. Starting configuration portal");
      /*
        AP_NONE = Continue executing code
        AP_LOOP = Trap in a continuous loop - Device is useless
        AP_RESET = Restart the chip
        AP_WAIT  = Trap in a continuous loop with captive portal until we have a working WiFi connection
      */
      wifiConnect.startConfigurationPortal(AP_WAIT);//if not connected show the configuration portal
    }
  } else {
    Serial.println("DEBUG WiFi already connected!");
  }

}

void setup() {

  btStop();
  // INITIATE SERIAL COMMUNICATION
  Serial.begin(115200);
    while (!Serial) 
    {
    }
  Serial.println("DEBUG Starting WiFi");

  Wire.begin(); 
  startWiFi();
  
  // filter
  ptr = ms_init(EMA);

}

void loop() {

  if ((WiFi.status() == WL_CONNECTED)) {
      startMillis=millis();                                             //save the starting time
      error = readCFSensor(0x6D);              //start conversion and read on pressure sensor ad 0x6D address
      while((millis()-startMillis) < period);                           //waits until period done
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

  // tmp just take min max and rescale positive only
  pressmap = map(press, -65.54, 65.54, 0.00, 131.08);
  
  // filter (ema filter) and scale for midi
  ccmap = map(ema_filter(pressmap, ptr), 0, 131.08, 0, 127);
  // don't use the filter but do scale for midi
  ccmapunfilter = map(press, -65.54, 65.54, 0, 127);

  
  //Send a OSC midi
  if(ccmapunfilter != cclast) {
    uint8_t midi[4];
    midi[0] = 0;
    midi[1] = 176;
    midi[2] = 11;
    midi[3] = ccmapunfilter;
    
    oscUdp.sendMessage("/midi",  "m",  midi);
    cclast = ccmapunfilter;
  }
  
  if(debugSerial) {
    Serial.println(String(ccmap) + " " + press);
  }
    
  return 0;
}