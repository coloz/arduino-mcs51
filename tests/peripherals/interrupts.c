#include "Arduino.h"
#define __SDCC 1
#define __sfr uint8_t
#define __at(address)
#define __reentrant
#include "stc_sfr.h"
#include "gpio.h"
#define STC_INTERRUPT_OK 0u
#define STC_INTERRUPT_INVALID 1u
#define STC_INTERRUPT_UNSUPPORTED_MODE 2u
uint8_t attachInterruptChecked(uint8_t,void (*)(void),int);
void (*stc_external_callbacks[2])(void);
#include "../../cores/STC/hal/WInterrupts.c"
static void callback(void) {}
int run_tests(void) {
 IE=0x82;TCON=0x80;
 CHECK(attachInterruptChecked(0,callback,FALLING)==0 && (TCON&1) && IE==0x83);
#if STC_CORE_INT01_MODE == 2
 CHECK(attachInterruptChecked(0,callback,LOW)==2 && (TCON&1));
 CHECK(attachInterruptChecked(0,callback,CHANGE)==0 && !(TCON&1));
#else
 CHECK(attachInterruptChecked(0,callback,CHANGE)==2 && (TCON&1));
 CHECK(attachInterruptChecked(0,callback,LOW)==0 && !(TCON&1));
#endif
 CHECK(attachInterruptChecked(0,callback,RISING)==2);
 CHECK(attachInterruptChecked(2,callback,FALLING)==1);
 CHECK(attachInterruptChecked(0,0,FALLING)==1);
 detachInterrupt(0);CHECK(IE==0x82 && !stc_external_callbacks[0]);
 return 0;
}
