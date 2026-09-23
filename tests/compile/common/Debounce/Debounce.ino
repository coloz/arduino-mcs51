const byte button = P3_3, led = P3_2;
int lastReading = HIGH, stable = HIGH;
unsigned long changedAt;
void setup() { pinMode(button, INPUT_PULLUP); pinMode(led, OUTPUT); }
void loop() {
  int reading = digitalRead(button);
  if (reading != lastReading) changedAt = millis();
  if (millis() - changedAt >= 50UL && reading != stable) {
    stable = reading;
    digitalWrite(led, stable == LOW ? HIGH : LOW);
  }
  lastReading = reading;
}
