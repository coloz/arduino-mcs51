#include <Wire.h>
#include <SPI.h>
volatile unsigned long edges;
void changed() { ++edges; }
void setup() { pinMode(P3_2, INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(P3_2), changed, FALLING); Wire.begin(); Wire.beginTransmission(0x50); Wire.write((uint8_t)0); Wire.endTransmission(); SPI.begin(); SPI.transfer(0x55); }
void loop() {
#if NUM_ANALOG_INPUTS > 0
  // Some supported families have no ADC, so they do not define A0.
  volatile int value = analogRead(A0);
  (void)value;
#endif
}
