#include "stc_peripheral_irqs.h"
#include "stc_isr_context.h"
STC_IRQ_DATA stc_uart_service_t stc_uart_extra_service;
#if STC_CORE_UART_COUNT >= 2
void stc_uart2_isr(void) __interrupt (8) {
    STC_ISR_CONTEXT_ENTER();
    if (stc_uart_extra_service) stc_uart_extra_service(2u);
    STC_ISR_CONTEXT_LEAVE();
}
#endif
#if STC_CORE_UART_COUNT >= 4
void stc_uart3_isr(void) __interrupt (17) {
    STC_ISR_CONTEXT_ENTER();
    if (stc_uart_extra_service) stc_uart_extra_service(3u);
    STC_ISR_CONTEXT_LEAVE();
}
void stc_uart4_isr(void) __interrupt (18) {
    STC_ISR_CONTEXT_ENTER();
    if (stc_uart_extra_service) stc_uart_extra_service(4u);
    STC_ISR_CONTEXT_LEAVE();
}
#endif
