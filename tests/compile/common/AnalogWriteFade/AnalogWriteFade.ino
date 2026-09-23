// The STC SDK currently implements digital threshold output, not PWM.
int brightness, delta = 5;
void setup() { pinMode(P3_2, OUTPUT); }
void loop() {
  analogWrite(P3_2, brightness);
  brightness += delta;
  if (brightness <= 0 || brightness >= 255) delta = -delta;
  delay(30);
}
