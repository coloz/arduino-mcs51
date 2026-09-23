const byte led = P3_2;
unsigned long previous;
bool lit;
void setup() { pinMode(led, OUTPUT); }
void loop() {
  unsigned long now = millis();
  if (now - previous >= 500UL) {
    previous = now;
    lit = !lit;
    digitalWrite(led, lit ? HIGH : LOW);
  }
}
