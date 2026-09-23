#include "Arduino.h"
#include "Wire_backend.h"
#include "stc_peripheral_irqs.h"
#include "gpio.h"
static uint8_t regs[10],mux,gate,gate_error,nack,timeout,received;
STC_IRQ_DATA stc_peripheral_service_t stc_wire_slave_service;
uint8_t stc_wire_hw_gate_read(void) { return gate; }
void stc_wire_hw_gate_write(uint8_t v) { gate=v; }
uint8_t stc_wire_hw_mux_read(void) { return mux; }
void stc_wire_hw_mux_write(uint8_t v) { mux=v; }
uint8_t stc_wire_hw_read(uint8_t r) { if(!(gate&128))gate_error=1;return regs[r]; }
void stc_wire_hw_write(uint8_t r,uint8_t v) {
    if(!(gate&128))gate_error=1;regs[r]=v;
    if(r==1 && v && !timeout) { regs[2]=(regs[2]&1)|0x40|(nack?2:0);if(v==4)regs[7]=0x60; }
}
static void receive(int n) { if(n==1)received=(uint8_t)Wire_read(); }
static void request(void) { Wire_write(0xa0); }
int run_tests(void) {
    mux=9;gate=0;
    CHECK(Wire_setPinsChecked(255,PIN_WIRE_SCL)==4);
    Wire_begin();CHECK(!Wire_usingHardware());
    Wire_beginTransmission(0x50);Wire_write(0xaa);CHECK(Wire_endTransmission()==2);
#if STC_CORE_I2C_COUNT
    CHECK(Wire_setPinsChecked(PIN_I2C1_SDA,PIN_I2C1_SCL)==0);CHECK(Wire_usingHardware());
    CHECK((regs[0]&0xc0)==0xc0 && gate==0);
    Wire_beginTransmission(0x50);Wire_write(0xaa);CHECK(Wire_endTransmissionStop(0)==0);
    CHECK(Wire_requestFromStop(0x50,2,1)==2);CHECK(Wire_read()==0x60 && Wire_read()==0x60);
    CHECK(regs[2]&1); /* Outgoing NACK must not be treated as incoming NACK. */
    Wire_beginTransmission(0x50);CHECK(Wire_endTransmission()==0);
    Wire_beginTransmission(0x50);CHECK(Wire_endTransmissionStop(0)==0);
    CHECK(Wire_requestFromStop(0x50,0,1)==0 && regs[1]==6);
    nack=1;Wire_beginTransmission(0x50);CHECK(Wire_endTransmission()==2);nack=0;
    Wire_setWireTimeout(2,1);timeout=1;Wire_beginTransmission(0x50);CHECK(Wire_endTransmission()==5);
    CHECK(Wire_getWireTimeoutFlag());timeout=0;Wire_clearWireTimeoutFlag();CHECK(!Wire_getWireTimeoutFlag());
    Wire_setClock(1000);CHECK(!Wire_usingHardware());Wire_setClock(100000);CHECK(Wire_usingHardware());
    Wire_end();CHECK(regs[0]==0 && mux==9);
    Wire_onReceive(receive);Wire_onRequest(request);Wire_beginSlave(0x30);
    CHECK(!Wire_configurationError() && stc_wire_slave_service);
    regs[4]=0x40;stc_wire_slave_service();regs[7]=0x60;regs[4]=0x20;stc_wire_slave_service();
    regs[7]=0x70;regs[4]=0x20;stc_wire_slave_service();regs[4]=8;stc_wire_slave_service();CHECK(received==0x70);
    regs[4]=0x40;stc_wire_slave_service();regs[7]=0x61;regs[4]=0x20;stc_wire_slave_service();CHECK(regs[6]==0xa0);
    Wire_end();CHECK(!stc_wire_slave_service && !gate_error && gate==0 && mux==9);
#else
    Wire_beginSlave(0x30);CHECK(Wire_configurationError()==4);CHECK(!stc_wire_slave_service);
    Wire_end();
#endif
    return 0;
}
