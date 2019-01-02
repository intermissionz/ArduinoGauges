#include <Arduino.h>
#include <CAN.h>
#include "Adafruit_SH1106.h"
#include "calibri8pt7b.h"

#define OBD_TIMEOUT 3
#define PID_ABSOLUTE_MANIFOLD_PRESSURE 0x0b
#define PID_AFR_LAMBDA 0x24
#define PID_BAROMETRIC_PRESSURE 0x33
#define PID_INTAKE_AIR_TEMP 0x0f
#define PID_OIL_TEMP 0x01
#define NORMAL_MODE 0x01
#define OIL_TEMP_MODE 0x21
#define NORMAL_RESPONSE 0x41
#define OIL_TEMP_RESPONSE 0x61
byte ethanolContent = 0;
Adafruit_SH1106 display(4);

void setup() {
  //Serial.begin(9600);
  //Serial.println("Setup Start");
  if (!CAN.begin(500E3)) {
    //Serial.println("Starting CAN failed!");
    while (1);
  } else {
    //Serial.println("CAN succeeded");
  }
  CAN.filter(0x7e8);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(1000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&calibri8pt7b);
  //Serial.println("setup done");
}

void loop() {
  display.clearDisplay();
  
  for (byte sensor = 0; sensor <= 5; sensor++) {
    DisplaySensorReading(sensor);
  }
  
  display.display();
  delay(2000);
}

void DisplaySensorReading(byte sensor) {
  byte x, y, precision;
  float value;
  char label[6];
  byte buffer[40];

  switch (sensor) {
    case 0: //oilPressure
      { //analog input 0
        x = 5;
        y = 15;
        precision = 0;
        strncpy(label, "OP: ", sizeof(label));

        double oilPressureVoltage = readAnalogInput(sensor, true);
        value = round(37.5 * oilPressureVoltage - 18.75);
        //Serial.println("OP passed");
        break;
      }
    case 1: //ethanolContent
      { //analog input 1
        x = 5;
        y = 35;
        precision = 0;
        strncpy(label, "E%: ", sizeof(label));

        float ethanolContentVoltage = readAnalogInput(sensor, true);
        value = round(ethanolContentVoltage * 20);
        ethanolContent = value; //afr calculation needs this so store it in a global
        //Serial.println("E% passed");
        break;
      }
    case 2: //afr
      { //obd - lambda pid 0x24
        x = 64;
        y = 35;
        precision = 1;
        strncpy(label, "AFR: ", sizeof(label));
        
        //float afrVoltage = readAnalogInput(sensor, true);
        //float afrLambda = 0.109364 * (afrVoltage * afrVoltage * afrVoltage) - 0.234466 * (afrVoltage * afrVoltage) + 0.306031 * afrVoltage + 0.71444;
        byte responseLength = canBusRequest(NORMAL_MODE, PID_AFR_LAMBDA, NORMAL_RESPONSE, buffer);
        if(responseLength > 1) {
          float afrLambda = 2 / 65536 * (256 * buffer[0] + buffer[1]);
          value = ((10 /*ethanolContent*/ / 100) * 9.0078 + (1 - (10 /*ethanolContent*/ / 100)) * 14.64) * afrLambda;
          //Serial.println("AFR passed");
        }
        break;
      }
    case 3: //boost
      { //obd - barometric pressure pid 0x33, absolute manifold pressure pid 0x0b
        x = 64;
        y = 55;
        precision = 1;
        strncpy(label, "BST: ", sizeof(label));

        //Serial.println("boost start");
        byte responseLength = canBusRequest(NORMAL_MODE, PID_BAROMETRIC_PRESSURE, NORMAL_RESPONSE, buffer);
        if(responseLength > 0) {
          byte barometricPressure = buffer[0];
          responseLength = canBusRequest(NORMAL_MODE, PID_ABSOLUTE_MANIFOLD_PRESSURE, NORMAL_RESPONSE, buffer);
          if(responseLength > 0) {
            byte absoluteManifoldPressure = buffer[0];
            byte boostkpa = absoluteManifoldPressure - barometricPressure;
            float boostMultiplier = 0.14503773800722; //default to psi multiplier
            if(boostkpa <= 0) {
              boostMultiplier = 0.29529983071445; //set to inHG multiplier if boost <= 0
            }
            value = boostkpa * boostMultiplier; 
          }
        }
        //Serial.println("BST passed");
        break;
      }
    case 4: //oilTemp
      /*{ //obd - pid 2101
        x = 64;
        y = 15;
        precision = 0;
        strncpy(label, "OT: ", sizeof(label));

        value = 125;
        byte responseLength = canBusRequest(OIL_TEMP_MODE, PID_OIL_TEMP, OIL_TEMP_RESPONSE, buffer);
        if(responseLength > 0) {
          byte oilTemp = buffer[28]; //value should be in the 29th position because 61 and 01 are stripped in the canBusRequest function
          value = round((oilTemp - 40) * 1.8 + 32);
        }
        //Serial.println("OT passed");
        break;
      }*/
      { // analog input 4
        float R1 = 11150, c1 = 1.143787531e-03, c2 = 2.305694337e-04, c3 = 0.7824155370e-07;
        x = 64;
        y = 15;
        precision = 0;
        strncpy(label, "OT: ", sizeof(label));
        
        int oilTempVoltage = readAnalogInput(sensor, true);
        float R2 = R1 * (1023.0 / (float)Vo - 1.0);
        float logR2 = log(R2);
        float T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2)); //Steinhart-Hart equation
        T = ((T - 273.15) * 9.0)/ 5.0 + 32.0; // convert K to C to F
        T = (T * 9.0)/ 5.0 + 32.0; // convert C to F
    case 5: //intakeAirTemp
      { //obd - pid 0x0f
        x = 5;
        y = 55;
        precision = 0;
        strncpy(label, "IAT: ", sizeof(label));
        byte responseLength = canBusRequest(NORMAL_MODE, PID_INTAKE_AIR_TEMP, NORMAL_RESPONSE, buffer);
        if(responseLength > 0) {
          byte intakeAirTemp = buffer[0];
          value = round((intakeAirTemp - 40) * 1.8 + 32);
        }
        //Serial.println("IAT passed");
        break;
      }
  }
  display.setCursor(x, y);
  display.print(label);
  display.print(value, precision);
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
  if (useMultiplier)
    value *= (5.0 / 1023.0);

  return value;
}

byte canBusRequest(const int mode, const int pid, const int responseType, byte* buffer) {
  byte i = 0;
  CAN.beginPacket(0x7df, 8);
  CAN.write(0x02);
  CAN.write(mode);
  CAN.write(pid);
  CAN.endPacket();
  
  while (CAN.parsePacket() == 0 || CAN.read() < 3 || CAN.read() != responseType || CAN.read() != pid); //wait for correct response

  while(CAN.available() && i <= 40) {
    buffer[i++] = CAN.read();
  }
  
  return i;
}
