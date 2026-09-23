void setup() { pinMode(P3_2, OUTPUT); pinMode(P3_3, INPUT); Serial.begin(2400); }
void loop() {
  digitalWrite(P3_2, LOW); delayMicroseconds(2);
  digitalWrite(P3_2, HIGH); delayMicroseconds(10);
  digitalWrite(P3_2, LOW);
  unsigned long duration = pulseIn(P3_3, HIGH, 30000UL);
  Serial.println(duration / 58UL);
  delay(100);
}
