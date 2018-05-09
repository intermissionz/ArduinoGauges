#include <CAN.h>

void setup() {
  Serial.begin(9600);
  Serial.println("CAN OBD-II PID test");
  CAN.begin(500E3));
  CAN.filter(0x7e0);
}

void loop() {
  CAN.beginPacket(0x7df, 8);
  CAN.write(0x02); // number of additional bytes
  CAN.write(0x21); // OBD mode
  CAN.write(0x01); // PID
  CAN.endPacket();
  Serial.println("packet sent");
  delay(10);
  Serial.println(CAN.parsePacket());
  do {
    Serial.println(CAN.read());
  } while (CAN.available()); //CAN.parsePacket() == 0 || CAN.read() < 3 || CAN.read() != 0x41 || CAN.read() != 0x33);
    
  /*float value = (CAN.read() - 40) * 1.8 + 32;
  Serial.print("Value = ");
  Serial.println(value);*/

  delay(2000);
}
