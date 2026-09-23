#include <Stepper.h>
Stepper motor(200, P3_0, P3_1, P3_2, P3_3);
void setup() { motor.setSpeed(30); }
void loop() { motor.step(200); delay(500); motor.step(-200); delay(500); }
