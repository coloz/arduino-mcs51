const byte dataPin = P3_2, clockPin = P3_3;
volatile uint8_t captured;
void setup() { pinMode(dataPin, OUTPUT); pinMode(clockPin, OUTPUT); }
void loop() {
  byte pattern = 0;
  bitSet(pattern, 3);
  bitWrite(pattern, 0, 1);
  bitToggle(pattern, 2);
  shiftOut(dataPin, clockPin, MSBFIRST, pattern);
  pinMode(dataPin, INPUT);
  captured = shiftIn(dataPin, clockPin, LSBFIRST);
  pinMode(dataPin, OUTPUT);
}
