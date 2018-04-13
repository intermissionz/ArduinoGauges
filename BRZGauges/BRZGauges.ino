/*libz*/
#include <Adafruit_GFX.h>
#include <EEPROM.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include <Wire.h>
/*localz*/
#include "sensor.h"
#include "Adafruit_SH1106.h"
#include "calibri8pt7b.h"
/*constantz*/
#define OLED_RESET 4
/*initializationz*/
Adafruit_SH1106 display(OLED_RESET);
Sensor oilPressureSensor = oilPressure;
Sensor afrSensor = afr;
Sensor ethanolContentSensor = ethanolContent;
Sensor boostSensor = boost;
Sensor oilTempSensor = oilTemp;
Sensor intakeAirTempSensor = intakeAirTemp;

void setup() {
  Serial.begin(9600);
  delay(200);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setFont(&calibri8pt7b);
  delay(5000);
}

void loop() {
  display.clearDisplay();
  DisplaySensorReading(oilPressureSensor);
  DisplaySensorReading(afrSensor);
  DisplaySensorReading(ethanolContentSensor);
  DisplaySensorReading(boostSensor);
  DisplaySensorReading(oilTempSensor);
  DisplaySensorReading(intakeAirTempSensor);
  display.display();
}

void DisplaySensorReading(Sensor sensor) {
  SensorData sensorData = GetSensorData(sensor);
  display.setCursor(sensorData.x, sensorData.y);
  display.print(sensorData.label);
  display.println(sensorData.displayValue);
  if(sensorData.x == 5) { //need to draw rectangle differently depending on which column the reading is in
    display.drawRect(sensorData.x - 5, sensorData.y - 15, 61, 21, 1);
    }
  else {
    display.drawRect(sensorData.x - 4, sensorData.y - 15, 68, 21, 1);
  }
  
}

SensorData GetSensorData(Sensor sensor) {
  SensorData sensorData;
  
  switch(sensor) {
    case oilPressure:
    {
      sensorData.x = 5;
      sensorData.y = 15;
      sensorData.label = "OP: ";
      
      double oilPressureVoltage = readAnalogInput(sensor, true);
      sensorData.value = -3.13608 * (oilPressureVoltage * oilPressureVoltage) + 51.4897 * oilPressureVoltage - 35.1307;
      sensorData.displayValue = String(round(sensorData.value), 0);
      sensorData.alert = 20 > sensorData.value; // arbitrary lower limit
      break;
    }
    case afr:
    {
      sensorData.x = 64;
      sensorData.y = 35;
      sensorData.label = "AFR: ";

      double afrVoltage = readAnalogInput(sensor, true);
      double afrLambda = 0.109364 * (afrVoltage * afrVoltage * afrVoltage) - 0.234466 * (afrVoltage * afrVoltage) + 0.306031 * afrVoltage + 0.71444;
      SensorData ethanolContent = GetSensorData(ethanolContentSensor);
      sensorData.value = ((ethanolContent.value / 100) * 9.0078 + (1 - (ethanolContent.value / 100)) * 14.64) * afrLambda;
      sensorData.displayValue = String(sensorData.value, 1);
      break;
    }
    case ethanolContent:
    {
      sensorData.x = 5;
      sensorData.y = 35;
      sensorData.label = "E%: ";

      double ethanolContentVoltage = readAnalogInput(sensor, true);
      sensorData.value = ethanolContentVoltage * 20;
      sensorData.displayValue = String(round(sensorData.value), 0);
      sensorData.alert = false;
      break;
    }
    case boost:
    {
      sensorData.x = 64;
      sensorData.y = 55;
      Serial.println("0133");
      delay(250);
      while(Serial.available() > 0) {
        sensorData.BTDataIn=0;
        sensorData.BTCharIn=0;
        sensorData.BTDataIn = Serial.read();
        sensorData.BTCharIn=char(sensorData.BTDataIn);
        sensorData.prePrecursorValue = sensorData.prePrecursorValue + sensorData.BTCharIn;
      }
      sensorData.precursorValue = sensorData.prePrecursorValue.substring(11,13);
      sensorData.tempValue = strtol(sensorData.precursorValue.c_str(),NULL,16); // atmospheric pressure in kPa
      Serial.println("010b");
      delay(250);
      while(Serial.available() > 0) {
        sensorData.BTDataIn=0;
        sensorData.BTCharIn=0;
        sensorData.BTDataIn = Serial.read();
        sensorData.BTCharIn=char(sensorData.BTDataIn);
        sensorData.prePrecursorValue = sensorData.prePrecursorValue + sensorData.BTCharIn;
      }
      sensorData.precursorValue = sensorData.prePrecursorValue.substring(11,13);
      sensorData.value = (strtol(sensorData.precursorValue.c_str(),NULL,16) - sensorData.tempValue) * .14503773773020923; // convert hex to decimal and kPa to psi
      sensorData.label = "BST: ";
      //sensorData.value = 24.8;
      sensorData.displayValue = String(sensorData.value, 1);
      sensorData.alert = sensorData.value >= 25;
      break;
    }
    case oilTemp:
    {
      sensorData.x = 64;
      sensorData.y = 15;
      sensorData.label = "OT: ";
      sensorData.value = 255.3;
      sensorData.displayValue = String(round(sensorData.value), 0);
      sensorData.alert = 170 >= sensorData.value || sensorData.value >= 230;
      break;
    }
    case intakeAirTemp:
    {
      sensorData.x = 5;
      sensorData.y = 55;
      sensorData.label = "IAT: ";
      sensorData.value = 100.2;
      sensorData.displayValue = String(round(sensorData.value), 0);
      sensorData.alert = sensorData.value > 150;
      break;
    }
  }

  return sensorData;
}

double readAnalogInput(Sensor sensor, bool useMultiplier) {
  double value;
  value = analogRead(sensor);
  if(useMultiplier)
    value = value * (5.0 / 1023.0); 

  return value;
}

/*void ReadBTData(Sensor sensor)
{
SensorData.prePrecursorValue="";  
  while(Serial.available() > 0)
  {
    SensorData.BTDataIn=0;
    SensorData.BTCharIn=0;
    SensorData.BTDataIn = Serial.read();
    SensorData.BTCharIn=char(BTDataIn);
    SensorData.prePrecursorValue = SensorData.prePrecursorValue + SensorData.BTCharIn;
  }
}*/
