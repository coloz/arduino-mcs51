#include <Wire.h>
void setup() { Wire.begin(); Serial.begin(2400); }
void loop() {
  for (byte address = 1; address < 127; ++address) {
    Wire.beginTransmission(address);
    byte result = Wire.endTransmission();
    if (result == 0) Serial.println(address, HEX);
  }
  delay(1000);
}
