#include "Arduino.h"

static unsigned long stc_random_state = 1UL;

void randomSeed(unsigned long seed)
{
    if (seed != 0UL) {
        stc_random_state = seed;
    }
}

static unsigned long stc_random_next(void)
{
    stc_random_state = stc_random_state * 1103515245UL + 12345UL;
    return stc_random_state;
}

long random(long upper_bound)
{
    if (upper_bound <= 0L) {
        return 0L;
    }
    return (long)(stc_random_next() % (unsigned long)upper_bound);
}

long random_minmax(long lower_bound, long upper_bound) STC_REENTRANT
{
    unsigned long span;
    unsigned long offset;

    if (lower_bound >= upper_bound) {
        return lower_bound;
    }
    /* A valid signed interval can span more than LONG_MAX values. Compute
     * its width without signed overflow, then convert only representable
     * offsets back to long (including when lower_bound is LONG_MIN). */
    span = (unsigned long)upper_bound - (unsigned long)lower_bound;
    offset = stc_random_next() % span;
    if (lower_bound < 0L) {
        unsigned long magnitude = 0UL - (unsigned long)lower_bound;
        if (offset >= magnitude) {
            return (long)(offset - magnitude);
        }
    }
    return lower_bound + (long)offset;
}

long map(long value, long from_low, long from_high,
         long to_low, long to_high) STC_REENTRANT
{
    if (from_high == from_low) {
        return to_low;
    }
    return (value - from_low) * (to_high - to_low) /
           (from_high - from_low) + to_low;
}
