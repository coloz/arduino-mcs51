#include "Arduino.h"
#include "gpio.h"
#define STC_SERIAL_HOST_TEST
static uint8_t xfr[4096], gate_error;
static uint8_t IE, IE2, P_SW2, AUXR, SCON, INTCLKO, BRT, UART_AUXR1;
static uint8_t S2CON,S2BUF,S3CON,S3BUF,S4CON,S4BUF,T4T3M,T2H,T2L,T3H,T3L,T4H,T4L;
static uint8_t *xr(unsigned long a) { if(!(P_SW2&128)) gate_error=1; return &xfr[a&4095u]; }
#define STC_XFR8(a) (*xr(a))
void stc_serial_test_transmit(uint8_t p,uint8_t v);
#include "../../cores/STC/hal/HardwareSerial_ports.c"
STC_IRQ_DATA stc_uart_service_t stc_uart_extra_service;
void stc_serial_test_transmit(uint8_t p,uint8_t v) {
    (void)v;
#if STC_CORE_UART_COUNT > 1
    control_write(p,control_read(p)|2u);
#else
    (void)p;
#endif
}
#if STC_CORE_UART_COUNT > 1
static void inject(uint8_t p,uint8_t v) {
    if(p==2) S2BUF=v; else if(p==3) S3BUF=v; else S4BUF=v;
    control_write(p,control_read(p)|1u);stc_uart_extra_service(p);
}
#endif
int run_tests(void) {
    uint8_t p,i,rx,tx,route;
    CHECK(!SerialPort_begin(0,9600));CHECK(!SerialPort_begin(5,9600));
    CHECK(!SerialPort_begin(2,0));CHECK(!SerialPort_begin(2,0xffffffffUL));
    CHECK(!SerialPort_begin(2,1));CHECK(!SerialPort_begin(2,2000000));
#if STC_CORE_UART_COUNT > 1
    IE=0x82;P_SW2=0x30;AUXR=0x10;
    CHECK(!SerialPort_begin(2,9600));CHECK(AUXR==0x10);
    AUXR=0;T2H=0x12;T2L=0x34;BRT=0x56;INTCLKO=7;UART_AUXR1=0xa5;
    xfr[(STC_CORE_UART_PS_BASE+2)&4095]=7;
    CHECK(SerialPort_begin(2,9600));CHECK(P_SW2==0x30 && IE==0x82);
#if defined(STC_CORE_FAMILY_12)
    CHECK((AUXR&0x1c)==0x1c && BRT==178 && S2CON==0x50);
#else
    CHECK((AUXR&0x1c)==0x14 && (((unsigned int)T2H<<8)|T2L)==65223);
#if STC_CORE_UART_TIMER_PRESCALER
    CHECK(xfr[(STC_CORE_UART_PS_BASE+2)&4095]==0);
#endif
#endif
    for(p=3;p<=STC_CORE_UART_COUNT;p++) CHECK(SerialPort_begin(p,9600));
    for(p=2;p<=STC_CORE_UART_COUNT;p++) inject(p,0x40+p);
    for(p=2;p<=STC_CORE_UART_COUNT;p++) {
        CHECK(SerialPort_available(p)==1 && SerialPort_peek(p)==0x40+p);
        CHECK(SerialPort_read(p)==0x40+p && SerialPort_read(p)==-1);
        for(i=0;i<32;i++) inject(p,i);
        CHECK(SerialPort_available(p)==SERIAL_RX_BUFFER_SIZE-1);
        CHECK(SerialPort_overflow(p) && !SerialPort_overflow(p));
        for(i=0;i<SERIAL_RX_BUFFER_SIZE-1;i++) CHECK(SerialPort_read(p)==i);
        inject(p,99);CHECK(SerialPort_read(p)==99);
        IE=2;CHECK(SerialPort_write(p,0x5a)==1 && IE==2);
        CHECK(!SerialPort_setPinsChecked(p,P3_0,P3_1));
        SerialPort_end(p);CHECK(!SerialPort_availableForWrite(p));
        if (p==3 && STC_CORE_UART_COUNT==4) CHECK((T4T3M&0xf0)==0xa0);
    }
    CHECK(!gate_error && AUXR==0 && T4T3M==0 && P_SW2==0x30 && INTCLKO==7);
#if defined(STC_CORE_FAMILY_12)
    CHECK(BRT==0x56 && UART_AUXR1==0xa5);
#else
    CHECK(T2H==0x12 && T2L==0x34);
#if STC_CORE_UART_TIMER_PRESCALER
    CHECK(xfr[(STC_CORE_UART_PS_BASE+2)&4095]==7);
#endif
#endif
    /* Exercise every complete alternate route, preserving unrelated fields. */
    for(p=2;p<=STC_CORE_UART_COUNT;p++) for(rx=0;rx<64;rx++) for(tx=0;tx<64;tx++) {
        route=route_for(p,rx,tx);
        if(route==255) continue;
        P_SW2=0x38;UART_AUXR1=0xa5;
        CHECK(SerialPort_setPinsChecked(p,rx,tx));
        CHECK(SerialPort_begin(p,9600));
#if defined(STC_CORE_FAMILY_12)
        CHECK(UART_AUXR1==(uint8_t)((0xa5&~0x10)|(route?0x10:0)));
#else
        CHECK(P_SW2==(uint8_t)(0x38|(route<<(p-2))));
#endif
        if(p==3) CHECK(T3H==0xfe && T3L==0xc7);
        if(p==4) CHECK(T4H==0xfe && T4L==0xc7);
        SerialPort_end(p);
        CHECK(P_SW2==0x38 && UART_AUXR1==0xa5);
    }
#else
    CHECK(!SerialPort_begin(2,9600));CHECK(SerialPort_read(2)==-1);
#endif
    return 0;
}
