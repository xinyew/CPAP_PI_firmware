#ifndef ARDUINO_COMPAT_H
#define ARDUINO_COMPAT_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef bool boolean;
typedef uint8_t byte;

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
