/*localz*/
#include "sensor.h"
/*libz*/
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include <EEPROM.h>
#include <math.h>
#include <SoftwareSerial.h>
#include <Timer.h>
#include <Wire.h>
/*constantz*/
#define OLED_RESET 4
/*initializationz*/
Adafruit_SH1106 display(OLED_RESET);
Sensor oilPressureSensor = oilPressure;
Sensor afrSensor = afr;
Sensor ethanolContentSensor = ethanolContent;

void setup() {
  Serial.begin(9600);
  delay(200);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void loop() {
  display.clearDisplay();
  DisplaySensorReading(oilPressureSensor);
  DisplaySensorReading(afrSensor);
  DisplaySensorReading(ethanolContentSensor);
  display.display();
}

void DisplaySensorReading(Sensor sensor) {
  SensorData sensorData = GetSensorData(sensor);
  display.setCursor(sensorData.x, sensorData.y);
  display.print(sensorData.label);
  display.println(sensorData.value);
}

SensorData GetSensorData(Sensor sensor) {
  SensorData sensorData;
  
  switch(sensor) {
    case oilPressure:
    {
      sensorData.x = 0;
      sensorData.y = 0;
      sensorData.label = "OP: ";
      
      double oilPressureVoltage = readAnalogInput(sensor, true);
      sensorData.value = -3.13608 * (oilPressureVoltage * oilPressureVoltage) + 51.4897 * oilPressureVoltage - 35.1307;
      break;
    }
    case afr:
    {
      sensorData.x = 0;
      sensorData.y = 16;
      sensorData.label = "AFR:";

      double afrVoltage = readAnalogInput(sensor, true);
      double afrLambda = 0.109364 * (afrVoltage * afrVoltage * afrVoltage) - 0.234466 * (afrVoltage * afrVoltage) + 0.306031 * afrVoltage + 0.71444;
      Sensor ethanolContentSensor = ethanolContent;
      SensorData ethanolContent = GetSensorData(ethanolContentSensor);
      sensorData.value = ((ethanolContent.value / 100) * 9.0078 + (1 - (ethanolContent.value / 100)) * 14.64) * afrLambda;
      break;
    }
    case ethanolContent:
    {
      sensorData.x = 0;
      sensorData.y = 32;
      sensorData.label = "E%: ";

      double ethanolContentVoltage = readAnalogInput(sensor, true);
      sensorData.value = ethanolContentVoltage * 20;
      break;
    }
    case boost:
    {
      break;
    }
    case oilTemp:
    {
      break;
    }
    case chargeTemp:
    {
      break;
    }
    default:
    {
      sensorData.x = 0;
      sensorData.y = 0;
      sensorData.label = "Unknown";
      sensorData.value = 0.0;
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