void setup() { Serial.begin(2400); }
void loop() {
  String line(" sensor=42 ");
  line.trim();
  int equal = line.indexOf('=');
  String key = line.substring(0, equal);
  long value = line.substring(equal + 1).toInt();
  key.toUpperCase();
  key.replace("SENSOR", "ADC");
  String result = key + ":" + String(value);
  if (result.startsWith("ADC")) Serial.println(result);
  delay(1000);
}
