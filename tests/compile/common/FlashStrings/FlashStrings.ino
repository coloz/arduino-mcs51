#include <pgmspace.h>
const uint8_t lookup[] PROGMEM = {3, 5, 8, 13};
const char message[] PROGMEM = "hello";
volatile byte index;
void setup() { Serial.begin(2400); }
void loop() {
  Serial.println(F("Flash string"));
  Serial.println(pgm_read_byte(&lookup[index & 3]));
  char buffer[8];
  strcpy_P(buffer, message);
  Serial.println(buffer);
  delay(1000);
}
