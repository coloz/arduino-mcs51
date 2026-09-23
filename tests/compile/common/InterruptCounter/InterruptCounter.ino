volatile unsigned long edges;
void onEdge() { ++edges; }
void setup() {
  Serial.begin(2400);
  pinMode(P3_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(P3_2), onEdge, FALLING);
}
void loop() {
  noInterrupts();
  unsigned long snapshot = edges;
  interrupts();
  Serial.println(snapshot);
  delay(1000);
}
