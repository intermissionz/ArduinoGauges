#include <Arduino.h>
#include <CAN.h>
#include <U8g2lib.h>

#define OBD_TIMEOUT 3
#define PID_ABSOLUTE_MANIFOLD_PRESSURE 0x0b
#define PID_BAROMETRIC_PRESSURE 0x33
#define PID_INTAKE_AIR_TEMP 0x0f
#define PID_OIL_TEMP 0x01
byte ethanolContent = 0;
U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI display(U8G2_R0, 10, 9, 8);

void setup() {
  delay(1000);
  display.setFont(u8g2_font_chroma48medium8_8u);
  display.setDrawColor(1); //not sure if this is needed. try 0 if text doesn't display
  while(!CAN.begin(500E3)); //wait until can bus is ready
  CAN.filter(0x7e8);
}

void loop() {
  display.clearDisplay();
  for (byte sensor = 0; sensor <= 5; sensor++) {
    DisplaySensorReading(sensor);
  }
  display.display();
}

void DisplaySensorReading(byte sensor) {
  byte x, y, precision;
  float displayValue;
  char label[6];

  switch (sensor) {
    case 0: //oilPressure
      { //analog input 0
        x = 5;
        y = 15;
        precision = 0;
        strncpy(label, "OP: ", sizeof(label));

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

        int barometricPressure = canBusRequest(PID_BAROMETRIC_PRESSURE);
        if(barometricPressure != 0) {
          int absoluteManifoldPressure = canBusRequest(PID_ABSOLUTE_MANIFOLD_PRESSURE);
          if(absoluteManifoldPressure != 0) {
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
        x = 64;
        y = 15;
        precision = 0;
        strcpy(label, "OT: ");

        displayValue = 125;
        break;
      }
    case 5: //intakeAirTemp
      { //obd - pid 0x0f
        x = 5;
        y = 55;
        precision = 0;
        strcpy(label, "IAT: ");

        int intakeAirTemp = canBusRequest(PID_INTAKE_AIR_TEMP);
        if(intakeAirTemp != 0) {
          displayValue = round((intakeAirTemp - 40) * 1.8 + 32);
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
  if (x == 5) { //need to draw rectangle differently depending on which column the reading is in
    display.drawFrame(x - 5, y - 15, 61, 21);
  }
  else {
    display.drawFrame(x - 4, y - 15, 68, 21);
  }
}

float readAnalogInput(byte sensor, bool useMultiplier) {
  float value = analogRead(sensor);
  if (useMultiplier)
    value *= (5.0 / 1023.0);

  return value;
}

int canBusRequest(const int pid) {
  byte responseTime = 0;
  CAN.beginPacket(0x7df, 8);
  CAN.write(0x02);
  CAN.write(0x01);
  CAN.write(pid);
  CAN.endPacket();
  int startTime = millis();
  
  while (CAN.parsePacket() == 0 || CAN.read() < 3 || CAN.read() != 0x41 || CAN.read() != pid || responseTime < OBD_TIMEOUT) { //wait for correct response
    responseTime = floor((millis() - startTime) / 1000); 
  }
  if(responseTime > OBD_TIMEOUT)
    return 0;
  else
    return CAN.read();
}
