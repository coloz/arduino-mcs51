#include "Arduino.h"
#include "stc_sfr.h"

#ifndef STC_TIMER0_CLOCK_DIVIDER
# if STC_CORE_TIMER1_IS_1T
#  define STC_TIMER0_CLOCK_DIVIDER 1UL
# else
#  define STC_TIMER0_CLOCK_DIVIDER 12UL
# endif
#endif

#define STC_TIMER0_TICKS_PER_MS \
    ((F_CPU + (STC_TIMER0_CLOCK_DIVIDER * 500UL)) / \
     (STC_TIMER0_CLOCK_DIVIDER * 1000UL))
#define STC_TIMER0_RELOAD (65536UL - STC_TIMER0_TICKS_PER_MS)

extern volatile unsigned long stc_timer0_millis_count;

static uint16_t stc_timer0_read(void)
{
    uint8_t high_before;
    uint8_t high_after;
    uint8_t low;

    do {
        high_before = TH0;
        low = TL0;
        high_after = TH0;
    } while (high_before != high_after);

    return ((uint16_t)high_after << 8) | low;
}

unsigned long micros(void)
{
    uint8_t enabled = IE & STC_IE_EA;
    uint8_t overflow;
    uint16_t counter;
    uint16_t elapsed_ticks;
    unsigned long milliseconds;

    IE &= (uint8_t)~STC_IE_EA;
    milliseconds = stc_timer0_millis_count;
    counter = stc_timer0_read();
    overflow = TCON & STC_TCON_TF0;

    if (overflow != 0u) {
        /* Re-read after observing TF0.  The first sample may have straddled
         * the overflow and therefore still belong to the preceding tick. */
        counter = stc_timer0_read();
        ++milliseconds;
#if STC_CORE_HAS_TIMER01_16BIT_AUTO_RELOAD
        elapsed_ticks = (uint16_t)(counter - (uint16_t)STC_TIMER0_RELOAD);
#else
        /* A classic mode-1 counter wraps to zero until the ISR reloads it. */
        elapsed_ticks = counter;
#endif
    } else {
        elapsed_ticks = (uint16_t)(counter - (uint16_t)STC_TIMER0_RELOAD);
    }

    if (enabled != 0u) {
        IE |= STC_IE_EA;
    }

    return milliseconds * 1000UL +
#if (STC_TIMER0_TICKS_PER_MS % 1000UL) == 0UL
        /* Integer timer ticks per microsecond: cancel the scale before
         * arithmetic so the 8051 needs only a 16-bit division. */
        elapsed_ticks / (uint16_t)(STC_TIMER0_TICKS_PER_MS / 1000UL);
#else
        ((uint32_t)elapsed_ticks * 1000UL) / STC_TIMER0_TICKS_PER_MS;
#endif
}

#if !STC_CORE_HAS_TIMER01_16BIT_AUTO_RELOAD
static unsigned long stc_timer0_sample(void)
{
    uint8_t enabled = IE & STC_IE_EA;
    uint8_t reloads;
    uint16_t counter;

    IE &= (uint8_t)~STC_IE_EA;
    /* A single byte detects every reload during the 65535 us API range.
     * If it wraps between samples, >= 256 ms have already elapsed, so
     * treating that interval as a raw delta cannot finish the wait early. */
    reloads = (uint8_t)stc_timer0_millis_count;
    counter = stc_timer0_read();
    if (enabled != 0u) {
        IE |= STC_IE_EA;
    }
    return ((unsigned long)reloads << 16) | counter;
}
#endif

void delayMicroseconds(unsigned int microseconds)
{
#if STC_TIMER0_TICKS_PER_MS <= 1000UL
    /* The full unsigned-int API range fits in 16 timer bits at <= 1 tick/us. */
    uint16_t remaining;
#else
    uint32_t remaining;
#endif
    uint16_t elapsed;
#if STC_CORE_HAS_TIMER01_16BIT_AUTO_RELOAD
    uint16_t previous;
    uint16_t current;
#else
    unsigned long previous;
    unsigned long current;
#endif

    if (microseconds == 0u) {
        return;
    }
    /* micros() can account for only one pending TF0. During a transaction
     * that masks interrupts, delays longer than one Timer0 period must not
     * depend on the ISR's millisecond count. Accumulate coherent counter
     * deltas instead. Timer0 must be running (as after init()). A preempting
     * ISR can lengthen this busy wait, but cannot make it return early.
     * Round up so fractional timer ticks do not shorten the requested wait.
     */
#if (STC_TIMER0_TICKS_PER_MS % 1000UL) == 0UL
    remaining = (uint32_t)microseconds * (STC_TIMER0_TICKS_PER_MS / 1000UL);
#else
    remaining = ((uint32_t)microseconds * STC_TIMER0_TICKS_PER_MS + 999UL) /
        1000UL;
#endif
#if STC_CORE_HAS_TIMER01_16BIT_AUTO_RELOAD
    previous = stc_timer0_read();
#else
    previous = stc_timer0_sample();
#endif
    for (;;) {
#if STC_CORE_HAS_TIMER01_16BIT_AUTO_RELOAD
        current = stc_timer0_read();
        if (current >= previous) {
            elapsed = current - previous;
        } else {
            elapsed = (uint16_t)(STC_TIMER0_TICKS_PER_MS -
                (uint16_t)(previous - current));
        }
#else
        current = stc_timer0_sample();
        if ((uint8_t)(current >> 16) == (uint8_t)(previous >> 16)) {
            /* No software reload: the raw timer free-runs modulo 65536. */
            elapsed = (uint16_t)current - (uint16_t)previous;
        } else {
            /* A mode-1 ISR discards ticks accumulated since overflow. A
             * normalized timestamp can therefore go backwards during the
             * reload. Count only time after the latest serviced reload;
             * discarding the uncertain interval can only extend the wait. */
            elapsed = (uint16_t)((uint16_t)current - (uint16_t)STC_TIMER0_RELOAD);
        }
#endif
        if (elapsed >= remaining) {
            break;
        }
        remaining -= elapsed;
        previous = current;
    }
}
