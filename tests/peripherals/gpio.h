static uint8_t levels[256], modes[256];
static unsigned long ticks;
static uint8_t software_reads, software_input = 0x96u, software_sent, software_lsb;
void pinMode(uint8_t p,uint8_t m) { modes[p]=m; }
void digitalWrite(uint8_t p,uint8_t v) { levels[p]=v; }
int digitalRead(uint8_t p) {
#ifdef TEST_SPI
    uint8_t mask=software_lsb ? (1u<<software_reads) : (0x80u>>software_reads);
    (void)p; if(levels[PIN_SPI_MOSI]) software_sent|=mask; ++software_reads;
    return (software_input&mask)?HIGH:LOW;
#else
    (void)p;return HIGH;
#endif
}
int digitalPinIsValid(uint8_t p) {
    static const uint8_t masks[]={PIN_VALID_MASK_P0,PIN_VALID_MASK_P1,PIN_VALID_MASK_P2,PIN_VALID_MASK_P3,PIN_VALID_MASK_P4,PIN_VALID_MASK_P5,PIN_VALID_MASK_P6,PIN_VALID_MASK_P7};
    return p<128u && (p&15u)<8u && (masks[p>>4]&(1u<<(p&7u)));
}
void delayMicroseconds(unsigned int d) { ticks+=d; }
unsigned long micros(void) { return ++ticks; }
#define CHECK(c) do { if(!(c)) return __LINE__; } while(0)
