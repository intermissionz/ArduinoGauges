/*omgz*/
#include <math.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Timer.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

#define OLED_RESET 4
#define VOLTAGE_MULTIPLIER (5.0 / 1023.0)
Adafruit_SSD1306 display(OLED_RESET);

void setup() {
  Serial.begin(9600);
  delay(200);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  delay(2000);
  display.setTextSize(2);
  display.setTextColor(WHITE);
}

void loop() {
  display.clearDisplay();
  display.setCursor(0,0);
  double analogOutput0 = analogRead(0);
  double airTemp = voltageToAirTemp(analogOutput0);
  double analogOutput1 = analogRead(1);
  double oilPressureVoltage = (analogOutput1 * VOLTAGE_MULTIPLIER);
  double analogOutput2 = analogRead(2);
  double ethanolContent = (analogOutput2 * VOLTAGE_MULTIPLIER * 20);
  //-3.13608 * (x*x) + 51.4897 * x - 35.1307
  double oilPressure = -3.13608 * (oilPressureVoltage * oilPressureVoltage) + 51.4897 * oilPressureVoltage - 35.1307;
  double analogOutput3 = analogRead(3);
  double lambdaVoltage = (analogOutput3 * VOLTAGE_MULTIPLIER);
  double lambda = .109364 * (lambdaVoltage * lambdaVoltage * lambdaVoltage) - .234466 * (lambdaVoltage * lambdaVoltage) + .306031 * lambdaVoltage + .71444;
  double AFR = (ethanolContent / 100) * 9.0078 + (1 - (ethanolContent / 100)) * 14.64;
  display.print("AT: ");
  display.println(airTemp);
  display.setCursor(0,16);
  display.print("OP: ");
  display.println(oilPressure);
  display.setCursor(0,32);
  display.print("AFR: ");
  display.println(AFR);
  display.setCursor(0,48);
  display.print("E%: ");
  display.println(ethanolContent);
  display.display();
}

double voltageToAirTemp(int RawADC) {  //Function to perform the fancy math of the Steinhart-Hart equation
 double Temp;
 Temp = log(((10240000/RawADC) - 10000));
 Temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * Temp * Temp ))* Temp );
 Temp = Temp - 273.15;              // Convert Kelvin to Celsius
 Temp = (Temp * 9.0)/ 5.0 + 32.0; // Celsius to Fahrenheit - comment out this line if you need Celsius
 return Temp;
}
