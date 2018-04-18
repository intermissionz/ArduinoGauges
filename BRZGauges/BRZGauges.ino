#include <Arduino.h>
#include <string.h>
#include "Adafruit_SH1106.h"
#include "calibri8pt7b.h"

Adafruit_SH1106 display(4);
byte ethanolContent = 0;
float barometricPressure = 0.0;

void setup() {
  Serial.begin(115200);
  delay(100);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(1000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&calibri8pt7b);
  //while (!obdInit()); //init connection until it succeeds
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
  char label[5];
  
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
      x = 64;
      y = 55;
      precision = 1;
      strcpy(label, "BST: ");
      
      if(barometricPressure <= 0) { //only read barometric pressure pid if its value hasn't already been set. pid: 0x33
        barometricPressure = 14;
      }
      int manifoldAbsolutePressure; //pid 0x0b
      int boostkpa = manifoldAbsolutePressure - barometricPressure;
      float boostMultiplier = 0.14503773800722; //default to psi multiplier
      if(boostkpa <= 0) {
        boostMultiplier = 0.29529983071445; //set to inHG multiplier if boost <= 0
      }
      displayValue = boostkpa * boostMultiplier;
      break;
    }
    case 4: //oilTemp
    { //obd - pid 2101
      x = 64;
      y = 15;
      precision = 0;
      strcpy(label, "OT: ");
      
      displayValue = 225;
      break;
    }
    case 5: //intakeAirTemp
    { //obd - pid 0x0f
      x = 5;
      y = 55;
      precision = 0;
      strcpy(label, "IAT: ");
      
      displayValue = 102;
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
