#include "Arduino.h"

#if defined(STCXX_CPP_CORE) && STCXX_CPP_CORE
# include "runtime/include/stcxx_runtime.h"
void __stcxx_heap_init(void);
#endif

/*
 * SDCC emits the interrupt vector table from declarations visible in the
 * translation unit that defines main().  The ISR implementations live in
 * separate core archive members, so declarations only at their definitions
 * are not sufficient to create the vector entries during the final link.
 */
#if STC_CORE_HAS_INT0
void stc_external0_isr(void) __interrupt (0);
#endif

void stc_timer0_isr(void) __interrupt (1);

#if STC_CORE_HAS_INT1
void stc_external1_isr(void) __interrupt (2);
#endif

#if STC_CORE_HAS_UART1 && STC_CORE_SERIAL_BUFFERED_RX
void stc_uart1_isr(void) __interrupt (4);
#endif

#if STC_CORE_UART_COUNT >= 2
void stc_uart2_isr(void) __interrupt (8);
#endif
#if STC_CORE_UART_COUNT >= 4
void stc_uart3_isr(void) __interrupt (17);
void stc_uart4_isr(void) __interrupt (18);
#endif
#if STC_CORE_I2C_COUNT > 0
void stc_wire_slave_isr(void) __interrupt (24);
#endif

int main(void)
{
#if defined(STCXX_CPP_CORE) && STCXX_CPP_CORE
    /*
     * SDCC startup has already initialized .data/.bss before entering main.
     * Initialize the board-sized XDATA heap before constructors because a
     * global C++ object is allowed to allocate.  The bridge then runs all
     * global C++ constructors before any Arduino lifecycle hook is observable.
     */
    __stcxx_heap_init();
    __stcxx_run_global_ctors();
#endif
    init();
    initVariant();
    setup();

    for (;;) {
        loop();
        yield();
    }
}
