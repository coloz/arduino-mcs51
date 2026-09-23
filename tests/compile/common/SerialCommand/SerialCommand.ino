void setup() { Serial.begin(2400); Serial.setTimeout(50); pinMode(P3_2, OUTPUT); }
void loop() {
  if (Serial.available()) {
    long value = Serial.parseInt();
    digitalWrite(P3_2, value ? HIGH : LOW);
    char buffer[8];
    size_t received = Serial.readBytesUntil('\n', buffer, sizeof(buffer));
    Serial.write((const uint8_t *)buffer, received);
    Serial.flush();
  }
}
