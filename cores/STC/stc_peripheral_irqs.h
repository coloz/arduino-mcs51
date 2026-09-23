#ifndef STC_PERIPHERAL_IRQS_H
#define STC_PERIPHERAL_IRQS_H
#include "Arduino.h"
#if defined(__SDCC)
#define STC_IRQ_DATA __data
#else
#define STC_IRQ_DATA
#endif
typedef void (*stc_peripheral_service_t)(void) STC_REENTRANT;
typedef void (*stc_uart_service_t)(uint8_t) STC_REENTRANT;
extern STC_IRQ_DATA stc_peripheral_service_t stc_wire_slave_service;
extern STC_IRQ_DATA stc_uart_service_t stc_uart_extra_service;
#endif
