#include "stc_peripheral_irqs.h"
#include "stc_isr_context.h"
STC_IRQ_DATA stc_peripheral_service_t stc_wire_slave_service;
#if STC_CORE_I2C_COUNT > 0
void stc_wire_slave_isr(void) __interrupt (24) {
    STC_ISR_CONTEXT_ENTER();
    if (stc_wire_slave_service) stc_wire_slave_service();
    STC_ISR_CONTEXT_LEAVE();
}
#endif
