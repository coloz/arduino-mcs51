#include "Arduino.h"
#include "HardwareSerial_private.h"
extern uint8_t stc_uart1_route;
extern void (*stc_uart1_pin_configure)(void) STC_REENTRANT;
static uint8_t serial_rx_pin, serial_tx_pin;
static void configure_custom_pins(void) STC_REENTRANT
{
    digitalWrite(serial_tx_pin, HIGH);
    pinMode(serial_rx_pin, INPUT_PULLUP);
    pinMode(serial_tx_pin, OUTPUT);
}
bool Serial_setPinsChecked(uint8_t rx, uint8_t tx)
{
    uint8_t route = STC_VARIANT_UART1_ROUTE(rx, tx);
    if (stc_uart1_started || route == 255u) return false;
    serial_rx_pin = rx; serial_tx_pin = tx; stc_uart1_route = route;
    if (rx == P3_0 && tx == P3_1) stc_uart1_pin_configure = 0;
    else stc_uart1_pin_configure = configure_custom_pins;
    return true;
}
