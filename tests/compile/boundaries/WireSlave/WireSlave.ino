#include <Wire.h>
void receiveEvent(int count) { while (Wire.available()) Wire.read(); }
void setup() { Wire.begin(8); Wire.onReceive(receiveEvent); }
void loop() {}
