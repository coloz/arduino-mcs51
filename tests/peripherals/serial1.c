#include "Arduino.h"
#define __SDCC 1
#define __sfr uint8_t
#define __at(address)
#define __reentrant
#include "stc_sfr.h"
#undef STC_XFR8
static uint8_t xfr[4096];
#define STC_XFR8(a) xfr[(a)&4095u]
#include "gpio.h"
volatile uint8_t stc_uart1_started,stc_uart1_rx_head,stc_uart1_rx_tail,stc_uart1_rx_overflow,stc_uart1_tx_complete;
uint8_t stc_uart1_rx_buffer[16],__stc_digital_input_pins[12];
void Serial_end(void);
bool Serial_beginChecked(unsigned long baud);
#include "../../cores/STC/hal/HardwareSerial.c"
#include "../../cores/STC/hal/HardwareSerial_pins.c"
int run_tests(void) {
    unsigned long rate=9600;
    uint8_t rx,tx,route;
#if defined(STC_CORE_FAMILY_89)
    rate=2400; /* 12 MHz / 12T cannot generate 9600 within 3%. */
#endif
    CHECK(!Serial_beginChecked(0));CHECK(!Serial_beginChecked(0xffffffffUL));
    CHECK(!stc_uart1_started);
    TMOD=0x12;TH1=0x12;TL1=0x34;TCON=0xc0;IE=0x8a;
#if STC_CORE_HAS_MODERN_UART1_BRT
    P_SW1=0x35;P_SW2=0x30;
#endif
    xfr[0xea1]=7;
    CHECK(Serial_beginChecked(rate));CHECK(stc_uart1_started);
    CHECK(!Serial_setPinsChecked(P3_0,P3_1));
    CHECK(!Serial_beginChecked(0) && stc_uart1_started);
#if STC_CORE_UART1_PRESCALER
    CHECK(xfr[0xea1]==0 && P_SW2==0x30);
#endif
    Serial_end();CHECK(!stc_uart1_started);
    CHECK(TMOD==0x12 && TH1==0x12 && TL1==0x34 && TCON==0xc0 && IE==0x8a);
    CHECK(xfr[0xea1]==7);
    CHECK(Serial_setPinsChecked(PIN_SERIAL1_RX,PIN_SERIAL1_TX));
    CHECK(!Serial_setPinsChecked(255,255));
    for(rx=0;rx<64;rx++) for(tx=0;tx<64;tx++) {
        route=STC_VARIANT_UART1_ROUTE(rx,tx);
        if(route==255) continue;
        CHECK(Serial_setPinsChecked(rx,tx));
        CHECK(Serial_beginChecked(rate));
#if STC_CORE_HAS_MODERN_UART1_BRT
        CHECK(P_SW1==(uint8_t)(0x35|(route<<6)));
#endif
        Serial_end();
#if STC_CORE_HAS_MODERN_UART1_BRT
        CHECK(P_SW1==0x35 && P_SW2==0x30);
#endif
    }
    return 0;
}
