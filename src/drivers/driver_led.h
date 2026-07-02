#ifndef DRIVER_LED_H
#define DRIVER_LED_H

#include <zephyr/kernel.h>

/* Board LEDs (active low: anode to DVDD, cathode via 1 k to MCU pin) */
#define LED_1    0   /* P1.08 */
#define LED_2    1   /* P1.09 */

/**
 * @brief Configure both status LEDs as outputs (off).
 *
 * @return 0 on success, negative errno on failure.
 */
int drv_led_init(void);

/**
 * @brief Set an LED on or off.
 *
 * @param idx  LED_1 or LED_2.
 * @param on   true = illuminated.
 * @return 0 on success, negative errno on failure.
 */
int drv_led_set(uint8_t idx, bool on);

/**
 * @brief Toggle an LED.
 *
 * @param idx  LED_1 or LED_2.
 * @return 0 on success, negative errno on failure.
 */
int drv_led_toggle(uint8_t idx);

#endif /* DRIVER_LED_H */
