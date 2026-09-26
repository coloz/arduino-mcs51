#ifndef STC_CORE_HARDWARE_SERIAL_PRIVATE_H
#define STC_CORE_HARDWARE_SERIAL_PRIVATE_H

#ifndef STC_CORE_SERIAL_BUFFERED_RX
# define STC_CORE_SERIAL_BUFFERED_RX 1
#endif

#if (STC_CORE_SERIAL_BUFFERED_RX != 0) && \
    (STC_CORE_SERIAL_BUFFERED_RX != 1)
# error "STC_CORE_SERIAL_BUFFERED_RX must be 0 or 1"
#endif

#ifndef SERIAL_RX_BUFFER_SIZE
# ifdef SERIAL_BUFFER_SIZE
#  define SERIAL_RX_BUFFER_SIZE SERIAL_BUFFER_SIZE
# else
#  define SERIAL_RX_BUFFER_SIZE 16u
# endif
#endif

#if STC_CORE_SERIAL_BUFFERED_RX && \
    ((SERIAL_RX_BUFFER_SIZE < 2) || (SERIAL_RX_BUFFER_SIZE > 255))
# error "SERIAL_RX_BUFFER_SIZE must be between 2 and 255 bytes"
#endif

#if STC_CORE_HAS_UART1
/* Five ISR/foreground hot bytes. Keep the RX payload in XDATA; callers can
 * trade speed for five more stack bytes with STC_SERIAL_STATE_IN_DATA=0. */
#ifndef STC_SERIAL_STATE_IN_DATA
# define STC_SERIAL_STATE_IN_DATA 1
#endif
#if STC_SERIAL_STATE_IN_DATA != 0 && STC_SERIAL_STATE_IN_DATA != 1
# error "STC_SERIAL_STATE_IN_DATA must be 0 or 1"
#endif
#if defined(__SDCC) && STC_SERIAL_STATE_IN_DATA
# define STC_SERIAL_HOT __data
#else
# define STC_SERIAL_HOT
#endif
extern STC_SERIAL_HOT volatile uint8_t stc_uart1_started;

# if STC_CORE_SERIAL_BUFFERED_RX
#  if defined(STC_XDATA_BYTES) && (STC_XDATA_BYTES > 0)
extern __xdata uint8_t stc_uart1_rx_buffer[SERIAL_RX_BUFFER_SIZE];
#  else
extern uint8_t stc_uart1_rx_buffer[SERIAL_RX_BUFFER_SIZE];
#  endif
extern STC_SERIAL_HOT volatile uint8_t stc_uart1_rx_head;
extern STC_SERIAL_HOT volatile uint8_t stc_uart1_rx_tail;
extern STC_SERIAL_HOT volatile uint8_t stc_uart1_rx_overflow;
extern STC_SERIAL_HOT volatile uint8_t stc_uart1_tx_complete;
# endif
#endif

#endif
