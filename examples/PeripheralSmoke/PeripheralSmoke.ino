#include <Wire.h>
#include <SPI.h>
volatile unsigned long edges;
void changed() { ++edges; }
void setup() { pinMode(P3_2, INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(P3_2), changed, FALLING); Wire.begin(); Wire.beginTransmission(0x50); Wire.write((uint8_t)0); Wire.endTransmission(); SPI.begin(); SPI.transfer(0x55); }
void loop() { volatile int value = analogRead(A0); (void)value; }
