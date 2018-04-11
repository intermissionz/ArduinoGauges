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
}

void loop() {
  display.clearDisplay();
  DisplaySensorReading(oilPressureSensor);
  DisplaySensorReading(afrSensor);
  DisplaySensorReading(ethanolContentSensor);
  DisplaySensorReading(boostSensor);
  DisplaySensorReading(oilTempSensor);
  DisplaySensorReading(intakeAirTempSensor);
  //display.drawFastHLine(0, 0, 128, 1);
  display.drawRect(0, 0, 128, 64, 1);
  display.display();
}

void DisplaySensorReading(Sensor sensor) {
  SensorData sensorData = GetSensorData(sensor);
  display.setCursor(sensorData.x, sensorData.y);
  display.print(sensorData.label);
  display.println(sensorData.displayValue);
}

SensorData GetSensorData(Sensor sensor) {
  SensorData sensorData;
  
  switch(sensor) {
    case oilPressure:
    {
      sensorData.x = 5;
      sensorData.y = 14;
      sensorData.label = "OP: ";
      
      double oilPressureVoltage = readAnalogInput(sensor, true);
      sensorData.value = -3.13608 * (oilPressureVoltage * oilPressureVoltage) + 51.4897 * oilPressureVoltage - 35.1307;
      sensorData.displayValue = String(round(sensorData.value), 0);
      break;
    }
    case afr:
    {
      sensorData.x = 64;
      sensorData.y = 36;
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
      sensorData.y = 36;
      sensorData.label = "E%: ";

      double ethanolContentVoltage = readAnalogInput(sensor, true);
      sensorData.value = ethanolContentVoltage * 20;
      sensorData.displayValue = String(round(sensorData.value), 0);
      break;
    }
    case boost:
    {
      sensorData.x = 64;
      sensorData.y = 58;
      sensorData.label = "BST: ";
      sensorData.value = 24.8;
      sensorData.displayValue = String(sensorData.value, 1);
      break;
    }
    case oilTemp:
    {
      sensorData.x = 64;
      sensorData.y = 14;
      sensorData.label = "OT: ";
      sensorData.value = 255.3;
      sensorData.displayValue = String(round(sensorData.value), 0);
      break;
    }
    case intakeAirTemp:
    {
      sensorData.x = 5;
      sensorData.y = 58;
      sensorData.label = "IAT: ";
      sensorData.value = 100.2;
      sensorData.displayValue = String(round(sensorData.value), 0);
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
