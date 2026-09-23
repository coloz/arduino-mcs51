#include <SPI.h>
volatile uint8_t received;
// Arduino pin constants must not replace library parameter identifiers.
uint8_t pinAliasParameter(uint8_t MOSI, uint8_t MISO, uint8_t SCK, uint8_t SS) {
 return MOSI ^ MISO ^ SCK ^ SS;
}
void setup() {
#if STC_CORE_SPI_COUNT
 SPI.setPinsChecked(PIN_SPI1_MOSI,PIN_SPI1_MISO,PIN_SPI1_SCK,PIN_SPI_SS);
#endif
 SPI.begin();SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE1));
 digitalWrite(PIN_SPI_SS,LOW);received=SPI.transfer(0x55);digitalWrite(PIN_SPI_SS,HIGH);
 SPI.endTransaction();received+=SPI.usingHardware();SPI.end();
}
void loop() {}
