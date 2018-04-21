#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <string.h>
#include "Adafruit_SH1106.h"
#include "calibri8pt7b.h"

#define OBD_TIMEOUT 3
#define PID_ABSOLUTE_MANIFOLD_PRESSURE 0x0b
#define PID_BAROMETRIC_PRESSURE 0x33
#define PID_INTAKE_AIR_TEMP 0x0f
#define PID_OIL_TEMP 0x01
Adafruit_SH1106 display(4);
byte ethanolContent = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(1000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&calibri8pt7b);
  while(obdInit()); //continue initializing until it successfully completes
}

void loop() {
  display.clearDisplay();
  for(byte sensor = 0; sensor <= 5; sensor++) {
    DisplaySensorReading(sensor);    
  }
  display.display();
}

void DisplaySensorReading(byte sensor) {
  byte x, y, precision;
  float displayValue;
  char label[6];
  
  switch(sensor) {
    case 0: //oilPressure
    { //analog input 0
      x = 5;
      y = 15;
      precision = 0;
      strcpy(label, "OP: ");
      
      double oilPressureVoltage = readAnalogInput(sensor, true);
      displayValue = round(-3.13608 * (oilPressureVoltage * oilPressureVoltage) + 51.4897 * oilPressureVoltage - 35.1307);
      break;
    }
    case 1: //ethanolContent
    { //analog input 1
      x = 5;
      y = 35;
      precision = 0;
      strcpy(label, "E%: ");
      
      float ethanolContentVoltage = readAnalogInput(sensor, true);
      displayValue = round(ethanolContentVoltage * 20);
      ethanolContent = displayValue; //afr calculation needs this so store it in a global
      break;
    }
    case 2: //afr
    { //analog input 2
      x = 64;
      y = 35;
      precision = 1;
      strcpy(label, "AFR: ");
      
      float afrVoltage = readAnalogInput(sensor, true);
      float afrLambda = 0.109364 * (afrVoltage * afrVoltage * afrVoltage) - 0.234466 * (afrVoltage * afrVoltage) + 0.306031 * afrVoltage + 0.71444;
      displayValue = ((ethanolContent / 100) * 9.0078 + (1 - (ethanolContent / 100)) * 14.64) * afrLambda;
      break;
    }
    case 3: //boost
    { //obd - barometric pressure pid 0x33, absolute manifold pressure pid 0x0b
      char obd_cmd[8], buffer[32];
      x = 64;
      y = 55;
      precision = 1;
      strcpy(label, "BST: ");

      sprintf(obd_cmd, "%02X%02X\r", 1, PID_BAROMETRIC_PRESSURE);
      byte responseLength = obdSendCommand(obd_cmd, buffer);

      if(responseLength > 0 && !strstr(buffer, "NO DATA")) {
        byte barometricPressure = obdParseResponse(PID_BAROMETRIC_PRESSURE, buffer);
  
        sprintf(obd_cmd, "%02X%02X\r", 1, PID_ABSOLUTE_MANIFOLD_PRESSURE);
        responseLength = obdSendCommand(obd_cmd, buffer);
        
        if(responseLength > 0 && !strstr(buffer, "NO DATA")) {
          byte absoluteManifoldPressure = obdParseResponse(PID_ABSOLUTE_MANIFOLD_PRESSURE, buffer);
          byte boostkpa = absoluteManifoldPressure - barometricPressure;
          float boostMultiplier = 0.14503773800722; //default to psi multiplier
          if(boostkpa <= 0) {
            boostMultiplier = 0.29529983071445; //set to inHG multiplier if boost <= 0
          }
          displayValue = boostkpa * boostMultiplier;
        }
      }
      break;
    }
    case 4: //oilTemp
    { //obd - pid 2101
      char obd_cmd[8], buffer[64];
      x = 64;
      y = 15;
      precision = 0;
      strcpy(label, "OT: ");

      sprintf(obd_cmd, "%02X%02X\r", 0x21, PID_OIL_TEMP);
      byte responseLength = obdSendCommand(obd_cmd, buffer);
      
      if(responseLength > 0 && !strstr(buffer, "NO DATA")) { //no idea how to parse this yet, let's see how long the obd response is
        displayValue = responseLength;
      }
      break;
    }
    case 5: //intakeAirTemp
    { //obd - pid 0x0f
      char obd_cmd[8], buffer[32];
      x = 5;
      y = 55;
      precision = 0;
      strcpy(label, "IAT: ");
      
      sprintf(obd_cmd, "%02X%02X\r", 1, PID_INTAKE_AIR_TEMP);
      byte responseLength = obdSendCommand(obd_cmd, buffer);
      
      if(responseLength > 0 && !strstr(buffer, "NO DATA")) {
        byte intakeAirTemp = obdParseResponse(PID_INTAKE_AIR_TEMP, buffer) - 40;
        displayValue = round(intakeAirTemp * 1.8 + 32);
      }
      break;
    }
  }

  display.setCursor(x, y);
  display.print(label);
  display.println(displayValue, precision);
  displayDrawRectangle(x, y);
}

void displayDrawRectangle(byte x, byte y) {
  if(x == 5) { //need to draw rectangle differently depending on which column the reading is in
    display.drawRect(x - 5, y - 15, 61, 21, 1);
  }
  else {
    display.drawRect(x - 4, y - 15, 68, 21, 1);
  }  
}

float readAnalogInput(byte sensor, bool useMultiplier) {
  float value = analogRead(sensor);
  if(useMultiplier)
    value *= (5.0 / 1023.0); 

  return value;
}

//on initialization, send commands ATZ -> ATE0 -> ATH0 -> ATSP6 -> AT SH 7E0
bool obdInit()
{
  const char *initcmd[] = {"ATZ\r", "ATE0\r", "ATH0\r", "ATSP6\r", "AT SH 7E0\r"};
  char buffer[32];

  for (byte i = (sizeof(initcmd) - 1); i != 0; i--) {
    byte responseLength = obdSendCommand(initcmd[i], buffer);
    if(responseLength <= 0 || strstr(buffer, "NO DATA")) {
      return false;
    }
  }
  return true;
}

int obdReceive(char* buffer, int buffersize)
{
  byte n = 0;

  while(Serial.available()) {
    char c = Serial.read();
    if (n < buffersize - 1) {
      if (c == '.' && n > 2 && buffer[n - 1] == '.' && buffer[n - 2] == '.') {
        // waiting for signal
        n = 0;
      } 
      else {
        if (c == '\r' || c == '\n' || c == ' ') {
          if (n == 0 || buffer[n - 1] == '\r' || buffer[n - 1] == '\n') continue;
        }
        buffer[n++] = c;
      }
    }
  }

  return n;
}

int obdSendCommand(const char* cmd, char* buffer)
{
  Serial.write(cmd);
  unsigned long startTime = millis();
  byte responseTime = 0;
  
  while(!Serial.available() && responseTime < OBD_TIMEOUT) { //wait for a response up to the OBD_TIMEOUT
    responseTime = floor((millis() - startTime) / 1000);
  }
  
  if(responseTime < OBD_TIMEOUT) { //request didn't time out 
     return obdReceive(buffer, sizeof(buffer));
  }
  
  return 0;
}

byte obdParseResponse(byte requestPID, char* buffer) {
  char *p = buffer;
  
  while ((p = strstr(p, "41 "))) {
    p += 3;
    byte responsePID = hex2byte(p);
    if (requestPID == responsePID) {
        p += 2;
        if (*p != ' ')
            return hex2byte(p);
    }
  }
  
  return 0;
}

byte hex2byte(const char *p)
{
  byte c1 = *p;
  byte c2 = *(p + 1);
  if (c1 >= 'A' && c1 <= 'F')
    c1 -= 7;
  else if (c1 >='a' && c1 <= 'f')
      c1 -= 39;
  else if (c1 < '0' || c1 > '9')
    return 0;

  if (c2 == 0)
    return (c1 & 0xf);
  else if (c2 >= 'A' && c2 <= 'F')
    c2 -= 7;
  else if (c2 >= 'a' && c2 <= 'f')
      c2 -= 39;
  else if (c2 < '0' || c2 > '9')
    return 0;

  return c1 << 4 | (c2 & 0xf);
}
