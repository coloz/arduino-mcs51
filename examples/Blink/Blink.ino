// Connect an LED and resistor to P3.2. No onboard LED is assumed.
void setup() { pinMode(P3_2, OUTPUT); }
void loop() { digitalWrite(P3_2, HIGH); delay(500); digitalWrite(P3_2, LOW); delay(500); }
