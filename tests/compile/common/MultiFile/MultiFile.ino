#include "helper.h"
void setup() { Serial.begin(2400); }
void loop() {
  Serial.println(scaleValue(cValue(3)));
  Serial.println(laterFunction(4));
  delay(1000);
}
int laterFunction(int input) { return input + 1; }
