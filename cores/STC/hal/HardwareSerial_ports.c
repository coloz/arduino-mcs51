/* UART2..4, 8N1, independent interrupt RX rings and synchronous TX.
 * STC12 UART2 owns BRT; STC15/STC8 UART2..4 own Timer2..4.
 * All register helpers shared with the ISR are reentrant. */
#include "Arduino.h"
#include "HardwareSerial_ports.h"
#include "HardwareSerial_private.h"
#include "stc_peripheral_irqs.h"
#ifndef STC_SERIAL_HOST_TEST
#include "stc_sfr.h"
__sfr __at (0x9a) S2CON;
__sfr __at (0x9b) S2BUF;
__sfr __at (0xac) S3CON;
__sfr __at (0xad) S3BUF;
__sfr __at (0x84) S4CON;
__sfr __at (0x85) S4BUF;
__sfr __at (0xd1) T4T3M;
__sfr __at (0xd4) T3H;
__sfr __at (0xd5) T3L;
__sfr __at (0xd2) T4H;
__sfr __at (0xd3) T4L;
__sfr __at (0xd6) T2H;
__sfr __at (0xd7) T2L;
__sfr __at (0xaf) IE2;
__sfr __at (0x8f) INTCLKO;
#if defined(STC_CORE_FAMILY_12)
__sfr __at (0x9c) BRT;
__sfr __at (0xa2) UART_AUXR1;
#endif
#endif

#if STC_CORE_UART_COUNT > 1
#define EXTRA_UARTS (STC_CORE_UART_COUNT - 1)
/* Keep rings and bookkeeping in the board's large-model XDATA. */
static volatile uint8_t uart_active[EXTRA_UARTS], uart_head[EXTRA_UARTS], uart_tail[EXTRA_UARTS];
static volatile uint8_t uart_overflow[EXTRA_UARTS], uart_tx_done[EXTRA_UARTS];
static uint8_t uart_custom[EXTRA_UARTS], uart_rx[EXTRA_UARTS], uart_tx[EXTRA_UARTS], uart_route[EXTRA_UARTS];
static uint8_t uart_timer_mode[EXTRA_UARTS], uart_saved_high[EXTRA_UARTS], uart_saved_low[EXTRA_UARTS];
static uint8_t uart_prescaler[EXTRA_UARTS], uart_clock_output[EXTRA_UARTS], uart_mux[EXTRA_UARTS];
static uint8_t uart_buffer[EXTRA_UARTS][SERIAL_RX_BUFFER_SIZE];

static uint8_t valid(uint8_t port)
{
    return port >= 2u && port <= STC_CORE_UART_COUNT &&
           (STC_CORE_UART_AVAILABLE_MASK & (1u << (port - 1u)));
}
static uint8_t route_for(uint8_t port, uint8_t rx, uint8_t tx)
{
    switch (port) {
    case 2: return STC_VARIANT_UART2_ROUTE(rx, tx);
    case 3: return STC_VARIANT_UART3_ROUTE(rx, tx);
    case 4: return STC_VARIANT_UART4_ROUTE(rx, tx);
    }
    return 255u;
}
static void default_pins(uint8_t port, uint8_t index)
{
    switch (port) {
    case 2: uart_rx[index] = PIN_SERIAL2_RX; uart_tx[index] = PIN_SERIAL2_TX; break;
    case 3: uart_rx[index] = PIN_SERIAL3_RX; uart_tx[index] = PIN_SERIAL3_TX; break;
    case 4: uart_rx[index] = PIN_SERIAL4_RX; uart_tx[index] = PIN_SERIAL4_TX; break;
    }
    uart_route[index] = route_for(port, uart_rx[index], uart_tx[index]);
}

/* EAXFR is held only for short register operations, never across TX waits. */
static uint8_t enter(void) STC_REENTRANT
{
#if STC_CORE_UART_TIMER_PRESCALER
    uint8_t saved = P_SW2 & 0x80u; P_SW2 |= 0x80u; return saved;
#else
    return 0u;
#endif
}
static void leave(uint8_t saved) STC_REENTRANT
{
#if STC_CORE_UART_TIMER_PRESCALER
    P_SW2 = (P_SW2 & 0x7fu) | saved;
#else
    (void)saved;
#endif
}
static uint8_t control_read(uint8_t port) STC_REENTRANT
{
    if (port == 2u) return S2CON;
    if (port == 3u) return S3CON;
    if (port == 4u) return S4CON;
    return 0u;
}
static void control_write(uint8_t port, uint8_t value) STC_REENTRANT
{
    if (port == 2u) S2CON = value;
    else if (port == 3u) S3CON = value;
    else if (port == 4u) S4CON = value;
}
static uint8_t data_read(uint8_t port) STC_REENTRANT
{
    if (port == 2u) return S2BUF;
    if (port == 3u) return S3BUF;
    if (port == 4u) return S4BUF;
    return 0u;
}
static void data_write(uint8_t port, uint8_t value)
{
    if (port == 2u) S2BUF = value;
    else if (port == 3u) S3BUF = value;
    else if (port == 4u) S4BUF = value;
}
static uint8_t irq_mask(uint8_t port)
{
    return port == 2u ? 1u : port == 3u ? 8u : 16u;
}
static void irq_enable(uint8_t port, uint8_t enabled)
{
    uint8_t mask = irq_mask(port);
    if (port <= 4u) IE2 = (IE2 & (uint8_t)~mask) | (enabled ? mask : 0u);
}
static void receive_interrupt(uint8_t port) STC_REENTRANT
{
    uint8_t index = port - 2u;
    uint8_t saved = enter(), status = control_read(port), next, value;
    if (status & 1u) {
        value = data_read(port);
        control_write(port, control_read(port) & (uint8_t)~1u);
        if (uart_active[index]) {
            next = uart_head[index] + 1u;
            if (next >= SERIAL_RX_BUFFER_SIZE) next = 0u;
            if (next == uart_tail[index]) uart_overflow[index] = 1u;
            else { uart_buffer[index][uart_head[index]] = value; uart_head[index] = next; }
        }
    }
    if (status & 2u) {
        control_write(port, control_read(port) & (uint8_t)~2u);
        uart_tx_done[index] = 1u;
    }
    leave(saved);
}

/* Snapshot only the selected timer's fields. Sibling timers and routes stay live. */
static uint8_t timer_busy(uint8_t port)
{
    if (port == 2u) return (AUXR & 0x10u) ||
#if !defined(STC_CORE_FAMILY_12)
        (IE2 & 4u) ||
#endif
        ((AUXR & 1u) && (SCON & 0x10u));
    if (port == 3u) return (T4T3M & 8u) || (IE2 & 0x20u);
    if (port == 4u) return (T4T3M & 0x80u) || (IE2 & 0x40u);
    return 1u;
}
static void timer_start(uint8_t port, uint8_t index, uint16_t reload)
{
    uint8_t mask = port == 3u || (port & 1u) ? 0x0fu : 0xf0u;
    if (port <= 4u) {
        #if STC_CORE_UART_TIMER_PRESCALER
        uart_prescaler[index] = STC_XFR8(STC_CORE_UART_PS_BASE + port);
        STC_XFR8(STC_CORE_UART_PS_BASE + port) = 0u;
#endif
        if (port == 2u) {
#if defined(STC_CORE_FAMILY_12)
            uart_timer_mode[index] = AUXR & 0x1cu; uart_saved_low[index] = BRT;
            uart_clock_output[index] = INTCLKO & 4u; INTCLKO &= (uint8_t)~4u;
            AUXR = (AUXR & (uint8_t)~0x1cu) | 0x0cu;
            BRT = (uint8_t)reload; AUXR |= 0x10u;
#else
            uart_timer_mode[index] = AUXR & 0x1cu; uart_saved_high[index] = T2H; uart_saved_low[index] = T2L;
            uart_clock_output[index] = INTCLKO & 4u; INTCLKO &= (uint8_t)~4u;
            AUXR = (AUXR & (uint8_t)~0x1cu) | 4u;
            T2H = reload >> 8; T2L = (uint8_t)reload; AUXR |= 0x10u;
#endif
        } else {
            uart_timer_mode[index] = T4T3M & mask; T4T3M &= (uint8_t)~mask;
            if (port == 3u) { uart_saved_high[index] = T3H; uart_saved_low[index] = T3L; T3H = reload >> 8; T3L = (uint8_t)reload; }
            else { uart_saved_high[index] = T4H; uart_saved_low[index] = T4L; T4H = reload >> 8; T4L = (uint8_t)reload; }
            T4T3M |= mask & 0xaau;
        }
    }
}
static void timer_stop(uint8_t port, uint8_t index)
{
    uint8_t mask = port == 3u || (port & 1u) ? 0x0fu : 0xf0u;
    if (port <= 4u) {
        if (port == 2u) {
            AUXR &= (uint8_t)~0x1cu;
#if defined(STC_CORE_FAMILY_12)
            BRT = uart_saved_low[index];
#else
            T2H = uart_saved_high[index]; T2L = uart_saved_low[index];
#endif
            INTCLKO = (INTCLKO & (uint8_t)~4u) | uart_clock_output[index];
            AUXR |= uart_timer_mode[index];
        } else {
            T4T3M &= (uint8_t)~mask;
            if (port == 3u) { T3H = uart_saved_high[index]; T3L = uart_saved_low[index]; }
            else { T4H = uart_saved_high[index]; T4L = uart_saved_low[index]; }
            T4T3M |= uart_timer_mode[index];
        }
        #if STC_CORE_UART_TIMER_PRESCALER
        STC_XFR8(STC_CORE_UART_PS_BASE + port) = uart_prescaler[index];
#endif
    }
}
static void mux_configure(uint8_t port, uint8_t index, uint8_t restore)
{
#if defined(STC_CORE_FAMILY_12)
    uint8_t value = UART_AUXR1 & 0x10u;
    (void)port;
    UART_AUXR1 = (UART_AUXR1 & 0xefu) | (restore ? uart_mux[index] : (uart_route[index] ? 0x10u : 0u));
#else
    uint8_t mask = 1u << (port - 2u), value = P_SW2 & mask;
    P_SW2 = (P_SW2 & (uint8_t)~mask) | (restore ? uart_mux[index] : (uart_route[index] ? mask : 0u));
#endif
    if (!restore) uart_mux[index] = value;
}
bool SerialPort_setPinsChecked(uint8_t port, uint8_t rx, uint8_t tx)
{
    uint8_t index;
    uint8_t route;
    if (!valid(port)) return false;
    index = port - 2u; route = route_for(port, rx, tx);
    if (uart_active[index] || route == 255u) return false;
    uart_rx[index] = rx; uart_tx[index] = tx; uart_route[index] = route; uart_custom[index] = 1u;
    return true;
}
bool SerialPort_begin(uint8_t port, unsigned long baud)
{
    uint8_t index;
    unsigned long divisor, denominator, ticks;
    uint8_t enabled, saved;
#if defined(STC_CORE_FAMILY_12)
    if (!valid(port) || !baud || baud > F_CPU / 16UL) return false;
    denominator = baud * 16UL;
#else
    if (!valid(port) || !baud || baud > F_CPU / 4UL) return false;
    denominator = baud * 4UL;
#endif
    divisor = (F_CPU + denominator / 2UL) / denominator;
#if defined(STC_CORE_FAMILY_12)
    if (!divisor || divisor > 256UL) return false;
#else
    if (!divisor || divisor > 65536UL) return false;
#endif
    ticks = divisor * denominator;
    /* |F_CPU - ticks| / ticks <= 3%. Constant bounds avoid another
     * 32-bit divide/modulo, and the split products cannot overflow. */
    if (ticks < (F_CPU / 103UL) * 100UL + ((F_CPU % 103UL) * 100UL + 102UL) / 103UL ||
        ticks > (F_CPU / 97UL) * 100UL + ((F_CPU % 97UL) * 100UL) / 97UL) return false;
    index = port - 2u;
    if (!uart_custom[index]) default_pins(port, index);
    if (uart_route[index] == 255u) return false;
    if (uart_active[index]) SerialPort_end(port);
    enabled = IE & 0x80u; IE &= 0x7fu; saved = enter();
    if (timer_busy(port) || (control_read(port) & 0x10u)) {
        leave(saved); IE |= enabled; return false;
    }
    irq_enable(port, 0u);
    mux_configure(port, index, 0u);
    digitalWrite(uart_tx[index], HIGH); pinMode(uart_tx[index], OUTPUT); pinMode(uart_rx[index], INPUT_PULLUP);
    uart_head[index] = uart_tail[index] = uart_overflow[index] = 0u; uart_tx_done[index] = 1u;
#if defined(STC_CORE_FAMILY_8)
    control_write(port, port == 2u ? 0x10u : 0x50u);
#else
    control_write(port, 0x50u);
#endif
    stc_uart_extra_service = receive_interrupt;
    uart_active[index] = 1u;
    timer_start(port, index, (uint16_t)(65536UL - divisor));
    irq_enable(port, 1u);
    leave(saved); IE |= enabled;
    return true;
}
void SerialPort_flush(uint8_t port)
{
    uint8_t index;
    uint8_t saved, enabled;
    if (!valid(port)) return;
    index = port - 2u;
    while (uart_active[index] && !uart_tx_done[index]) {
        enabled = IE & 0x80u; IE &= 0x7fu; saved = enter();
        if (control_read(port) & 2u) { control_write(port, control_read(port) & (uint8_t)~2u); uart_tx_done[index] = 1u; }
        leave(saved); IE |= enabled;
    }
}
void SerialPort_end(uint8_t port)
{
    uint8_t index;
    uint8_t enabled, saved;
    if (!valid(port)) return;
    index = port - 2u; if (!uart_active[index]) return;
    SerialPort_flush(port);
    enabled = IE & 0x80u; IE &= 0x7fu; saved = enter();
    irq_enable(port, 0u); control_write(port, 0u); uart_active[index] = 0u;
    timer_stop(port, index); mux_configure(port, index, 1u);
    uart_head[index] = uart_tail[index] = uart_overflow[index] = 0u;
    pinMode(uart_rx[index], INPUT); pinMode(uart_tx[index], INPUT);
    leave(saved); IE |= enabled;
}
int SerialPort_available(uint8_t port)
{
    uint8_t index; uint8_t head, tail;
    if (!valid(port)) return 0;
    index = port - 2u; if (!uart_active[index]) return 0;
    head = uart_head[index]; tail = uart_tail[index];
    return head >= tail ? head - tail : SERIAL_RX_BUFFER_SIZE - tail + head;
}
int SerialPort_peek(uint8_t port)
{
    uint8_t index;
    if (!SerialPort_available(port)) return -1;
    index = port - 2u; return uart_buffer[index][uart_tail[index]];
}
int SerialPort_read(uint8_t port)
{
    uint8_t index; uint8_t tail; int value = SerialPort_peek(port);
    if (value < 0) return -1;
    index = port - 2u; tail = uart_tail[index] + 1u;
    if (tail >= SERIAL_RX_BUFFER_SIZE) tail = 0u;
    uart_tail[index] = tail; return value;
}
int SerialPort_availableForWrite(uint8_t port)
{
    return valid(port) && uart_active[port - 2u] && uart_tx_done[port - 2u] ? 1 : 0;
}
size_t SerialPort_write(uint8_t port, uint8_t value)
{
    uint8_t enabled, saved;
    if (!valid(port) || !uart_active[port - 2u]) return 0u;
    SerialPort_flush(port);
    enabled = IE & 0x80u; IE &= 0x7fu; saved = enter();
    uart_tx_done[port - 2u] = 0u;
    control_write(port, control_read(port) & (uint8_t)~2u); data_write(port, value);
#ifdef STC_SERIAL_HOST_TEST
    stc_serial_test_transmit(port, value);
#endif
    leave(saved); IE |= enabled;
    SerialPort_flush(port); return 1u;
}
bool SerialPort_overflow(uint8_t port)
{
    uint8_t enabled, value;
    if (!valid(port)) return false;
    enabled = IE & 0x80u; IE &= 0x7fu;
    value = uart_overflow[port - 2u]; uart_overflow[port - 2u] = 0u;
    IE |= enabled; return value != 0u;
}

#else /* Targets exposing UART1 only: unavailable numbered objects are inert. */
bool SerialPort_begin(uint8_t p, unsigned long b) { (void)p; (void)b; return false; }
void SerialPort_end(uint8_t p) { (void)p; }
bool SerialPort_setPinsChecked(uint8_t p,uint8_t r,uint8_t t) { (void)p;(void)r;(void)t;return false; }
int SerialPort_available(uint8_t p) { (void)p;return 0; }
int SerialPort_availableForWrite(uint8_t p) { (void)p;return 0; }
int SerialPort_peek(uint8_t p) { (void)p;return -1; }
int SerialPort_read(uint8_t p) { (void)p;return -1; }
void SerialPort_flush(uint8_t p) { (void)p; }
size_t SerialPort_write(uint8_t p,uint8_t v) { (void)p;(void)v;return 0; }
bool SerialPort_overflow(uint8_t p) { (void)p;return false; }
#endif
