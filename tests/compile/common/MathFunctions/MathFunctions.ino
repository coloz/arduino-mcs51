volatile float input = 1.25f;
volatile float result;
volatile long mapped;
void setup() { randomSeed(analogRead(A0)); }
void loop() {
  float x = input;
  result = sqrt(x) + sin(x) + cos(x) + pow(x, 2.0f) + log(x) + exp(x);
  result += fabs(x) + floor(x) + ceil(x);
  mapped = constrain(map(analogRead(A0), 0, 1023, 0, 255), 0L, 255L);
  mapped += random(10, 100);
}
