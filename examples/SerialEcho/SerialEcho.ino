void setup() { Serial.begin(2400); Serial.println("STC MCS51"); }
void loop() { if (Serial.available()) Serial.write(Serial.read()); }
