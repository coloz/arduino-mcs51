#define TEST_SPI
#include "Arduino.h"
#include "SPI_backend.h"
#include "gpio.h"
static uint8_t regs[4],mux,gate,interrupt_state,timeout,collision,gate_error;
uint8_t stc_spi_host_interrupt_state_read(void) { return interrupt_state; }
void stc_spi_host_interrupt_state_write(uint8_t v) { interrupt_state=v; }
uint8_t stc_spi_hw_gate_read(void) { return gate; }
void stc_spi_hw_gate_write(uint8_t v) { gate=v; }
uint8_t stc_spi_hw_read(uint8_t b,uint8_t r) { (void)b;if(r==3 && !(gate&128))gate_error=1;return regs[r]; }
void stc_spi_hw_write(uint8_t b,uint8_t r,uint8_t v) {
    (void)b;if(r==3 && !(gate&128))gate_error=1;
    if(r==1)regs[1]&=~v;
    else if(r==2) { regs[2]=v^0x5a; if(!timeout)regs[1]|=0x80; if(collision)regs[1]|=0x40; }
    else regs[r]=v;
}
uint8_t stc_spi_hw_mux_read(uint8_t b) { (void)b;return mux; }
void stc_spi_hw_mux_write(uint8_t b,uint8_t v) { (void)b;mux=v; }
int run_tests(void) {
    uint8_t mode,order,i;
    static const uint8_t divs[]={STC_CORE_SPI_DIV0,STC_CORE_SPI_DIV1,STC_CORE_SPI_DIV2,STC_CORE_SPI_DIV3};
    interrupt_state=0xa5;mux=0xa6;gate=0x31;regs[3]=0x55;
    CHECK(SPI_setPinsChecked(255,PIN_SPI_MISO,PIN_SPI_SCK,PIN_SPI_SS)==STC_SPI_INVALID);
    CHECK(SPI_setPinsChecked(PIN_SPI_MOSI,PIN_SPI_MOSI,PIN_SPI_SCK,PIN_SPI_SS)==STC_SPI_INVALID);
    SPI_begin();CHECK(!SPI_usingHardware());
    for(mode=0;mode<4;mode++) for(order=0;order<2;order++) {
        SPI_setSettings(100000,order,mode);software_reads=software_sent=0;software_lsb=order==LSBFIRST;
        CHECK(SPI_transfer(0xa6)==0x96 && software_sent==0xa6 && software_reads==8);
    }
#if STC_CORE_SPI_COUNT
    CHECK(SPI_setPinsChecked(PIN_SPI1_MOSI,PIN_SPI1_MISO,PIN_SPI1_SCK,PIN_SPI1_SS)==0);
    for(mode=0;mode<4;mode++) for(order=0;order<2;order++) for(i=0;i<4;i++) if(divs[i]) {
        SPI_setSettings(F_CPU/divs[i],order,mode);
#if defined(STC_CORE_FAMILY_12) || defined(STC_CORE_FAMILY_15)
        if (!(mode & 1u)) { CHECK(!SPI_usingHardware()); continue; }
#endif
        CHECK(SPI_usingHardware());
        CHECK(regs[0]==(0xd0u|(order==LSBFIRST?0x20u:0u)|(mode<<2)|i));
        CHECK(SPI_transfer(0xa6)==(0xa6^0x5a) && regs[1]==0);
    }
    SPI_setSettings(1,MSBFIRST,0);CHECK(!SPI_usingHardware() && mux==0xa6);
    SPI_setSettings(10000000,MSBFIRST,1);CHECK(SPI_usingHardware());
#endif
    SPI_usingInterrupt(0);CHECK(SPI_beginTransactionChecked(1000000,MSBFIRST,0)==0);
    CHECK(interrupt_state==0xa4);
    CHECK(SPI_beginTransactionChecked(1000000,MSBFIRST,0)==STC_SPI_BUSY);
    CHECK(interrupt_state==0xa4);SPI_endTransaction();CHECK(interrupt_state==0xa5);
    CHECK(SPI_beginTransactionChecked(0,MSBFIRST,0)==STC_SPI_INVALID);
#if STC_CORE_SPI_COUNT
    SPI_setSettings(10000000,MSBFIRST,1);collision=1;
    CHECK(SPI_transfer(1)==255 && SPI_configurationError()==STC_SPI_BUSY);
    collision=0;SPI_setSettings(10000000,MSBFIRST,1);timeout=1;
    CHECK(SPI_transfer(1)==255 && SPI_configurationError()==STC_SPI_TIMEOUT && !SPI_usingHardware());
#endif
    SPI_end();CHECK(mux==0xa6 && gate==0x31 && regs[3]==0x55 && !gate_error);
    return 0;
}
