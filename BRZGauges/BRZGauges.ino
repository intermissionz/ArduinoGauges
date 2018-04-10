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
#define VOLTAGE_MULTIPLIER (5.0 / 1023.0)
/*initializationz*/
Adafruit_SH1106 display(OLED_RESET);
Sensor oilPressureSensor = oilPressure;
Sensor afrSensor = afr;
Sensor ethanolContentSensor = ethanolContent;

void setup() {
  Serial.begin(9600);
  delay(200);
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  delay(2000);
  display.setTextSize(2);
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 16);
  display.print("Test ");
  display.println(1.23);
  //DisplaySensorReading(oilPressureSensor);
  //DisplaySensorReading(afrSensor);
  //DisplaySensorReading(ethanolContentSensor);
  display.display();
}

void DisplaySensorReading(Sensor sensor) {
  //SensorData sensorData = GetSensorData(sensor);
  display.setCursor(0, 16);
  display.print("Test ");
  display.println(1.23);
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
    value = value * VOLTAGE_MULTIPLIER; 

  return value;
}

double steinhartHartEquation(int RawADC) { //Function to perform the fancy math of the Steinhart-Hart equation
 double Temp;
 Temp = log(((10240000/RawADC) - 10000));
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;            // Convert Kelvin to Celsius
 Temp = (Temp * 9.0)/ 5.0 + 32.0; // Celsius to Fahrenheit - comment out this line if you need Celsius
 return Temp;
}

/* old stuffz
 *  
 * double analogOutput0 = analogRead(0);
 * double airTemp = steinhartHartEquation(analogOutput0);
 */
