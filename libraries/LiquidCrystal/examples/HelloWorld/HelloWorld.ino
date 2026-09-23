/* SPDX-License-Identifier: MIT */

#include <Arduino.h>
#include <LiquidCrystal.h>

/* Connect R/W to ground.  Change these pins to match the target board. */
#define LCD_RS P3_0
#define LCD_EN P3_1
#define LCD_D4 P3_2
#define LCD_D5 P3_3
/* These aliases select P3.4/P3.5, or P5.4/P5.5 on STC8G1K08A.
 * Do not use UART, interrupts or SPI on the same pins while driving the LCD. */
#define LCD_D6 PIN_SPI_SCK
#define LCD_D7 PIN_SPI_SS

LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup(void)
{
    lcd.begin(16u, 2u);
    lcd.print("hello, world!");
    lcd.setCursor(0u, 1u);
    lcd.print("arduino-stc51");
}

void loop(void)
{
}
