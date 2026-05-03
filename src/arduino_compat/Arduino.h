#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool boolean;
typedef uint8_t byte;

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif

static inline uint32_t millis(void) {
    return k_uptime_get_32();
}

static inline void delay(uint32_t ms) {
    k_msleep(ms);
}

#ifdef __cplusplus
}
#endif

#endif /* ARDUINO_COMPAT_H */
