void setup() { Serial.begin(2400); analogReference(DEFAULT); }
void loop() {
  int reading = analogRead(A0);
  Serial.println(reading);
  delay(100);
}
