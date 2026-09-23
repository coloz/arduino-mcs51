#include <Arduino.h>
void setup() {
 Serial.begin(2400); Serial.write((uint8_t)0x55);
#if STC_CORE_UART_COUNT >= 2
 Serial2.setPinsChecked(PIN_SERIAL2_RX,PIN_SERIAL2_TX); Serial2.begin(9600); Serial2.write((uint8_t)0x55);
#endif
#if STC_CORE_UART_COUNT >= 4
 Serial3.begin(19200); Serial4.begin(38400);Serial3.write((uint8_t)0x33);Serial4.write((uint8_t)0x44);
#endif
}
void loop() {
#if STC_CORE_UART_COUNT >= 2
 if(Serial2.available()) Serial2.write(Serial2.read());
#endif
}
