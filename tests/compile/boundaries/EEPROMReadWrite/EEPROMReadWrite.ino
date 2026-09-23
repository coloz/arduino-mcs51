#include <EEPROM.h>
void setup() { EEPROM.update(0, EEPROM.read(0) + 1); }
void loop() {}
