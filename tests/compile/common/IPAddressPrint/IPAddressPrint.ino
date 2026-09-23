IPAddress address(192, 168, 1, 51);
void setup() { Serial.begin(2400); }
void loop() {
  IPAddress parsed;
  if (parsed.fromString("10.0.0.5")) {
    parsed[3] = address[3];
    Serial.println(parsed);
    Serial.println(parsed == address);
  }
  delay(1000);
}
