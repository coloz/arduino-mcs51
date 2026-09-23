#include <Wire.h>
volatile uint8_t status;
void setup() {
#if STC_CORE_I2C_COUNT
 Wire.setPinsChecked(PIN_I2C1_SDA,PIN_I2C1_SCL);
#endif
 Wire.begin();Wire.setClock(100000);Wire.setWireTimeout(25000,true);
 Wire.beginTransmission(0x50);Wire.write((uint8_t)0);status=Wire.endTransmission(false);
 Wire.requestFrom((uint8_t)0x50,(size_t)1);status+=Wire.usingHardware();Wire.end();
}
void loop() {}
