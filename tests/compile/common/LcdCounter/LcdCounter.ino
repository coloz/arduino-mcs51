#include <LiquidCrystal.h>
// These pins must be adapted to the actual package before hardware use.
LiquidCrystal lcd(P3_0, P3_1, P3_2, P3_3, PIN_SPI_SCK, PIN_SPI_SS);
void setup() { lcd.begin(16, 2); lcd.print("Arduino STC"); }
void loop() { lcd.setCursor(0, 1); lcd.print(millis() / 1000UL); delay(1000); }
