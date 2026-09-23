#include <Servo.h>
Servo servo;
void setup() { servo.attach(P3_2); }
void loop() { servo.write(90); delay(1000); }
