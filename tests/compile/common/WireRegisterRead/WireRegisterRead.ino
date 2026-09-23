#include <Wire.h>
void setup() { Wire.begin(); Wire.setClock(100000); Serial.begin(2400); }
void loop() {
  Wire.beginTransmission(0x50);
  Wire.write((uint8_t)0x10);
  if (Wire.endTransmission(false) == 0) {
    Wire.requestFrom((uint8_t)0x50, (uint8_t)2);
    while (Wire.available()) Serial.write(Wire.read());
  }
  delay(1000);
}
