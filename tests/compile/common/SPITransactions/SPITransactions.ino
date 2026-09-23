#include <SPI.h>
uint8_t data[] = {0x9f, 0, 0, 0};
void setup() { SPI.begin(); pinMode(SS, OUTPUT); }
void loop() {
  SPI.beginTransaction(SPISettings(100000UL, MSBFIRST, SPI_MODE0));
  digitalWrite(SS, LOW);
  SPI.transfer(data, sizeof(data));
  uint16_t response = SPI.transfer16(0xabcd);
  digitalWrite(SS, HIGH);
  SPI.endTransaction();
  data[0] = lowByte(response);
}
