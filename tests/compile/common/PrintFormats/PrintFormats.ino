struct Reading : Printable {
  size_t printTo(Print &out) const { return out.print("value=") + out.print(42); }
};
Reading reading;
void setup() { Serial.begin(2400); }
void loop() {
  Serial.println(255, HEX);
  Serial.println(-12345L);
  Serial.println(3.14159, 3);
  Serial.println(true);
  Serial.println(reading);
  Serial.write((const uint8_t *)"OK\n", 3);
  delay(1000);
}
