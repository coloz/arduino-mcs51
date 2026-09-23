#include <SoftwareSerial.h>
// This SDK polls RX; calls to available() must be frequent.
SoftwareSerial port(P3_2, P3_3);
void setup() { Serial.begin(2400); port.begin(9600); }
void loop() {
  if (port.available()) Serial.write(port.read());
  if (Serial.available()) port.write(Serial.read());
}
