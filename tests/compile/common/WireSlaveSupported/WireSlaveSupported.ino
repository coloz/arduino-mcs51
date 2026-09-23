#include <Wire.h>
volatile uint8_t value;
void receiveData(int count) { if(count && Wire.available())value=(uint8_t)Wire.read(); }
void requestData() { Wire.write(value); }
void setup() {
#if WIRE_HAS_SLAVE
 Wire.onReceive(receiveData);Wire.onRequest(requestData);Wire.begin((uint8_t)0x30);
#else
 Wire.begin();
#endif
}
void loop() {}
