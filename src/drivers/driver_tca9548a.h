#ifndef DRIVER_TCA9548A_H
#define DRIVER_TCA9548A_H

#include <zephyr/kernel.h>

/* TCA9548A I2C address (A0 = A1 = A2 = GND) */
#define TCA9548A_I2C_ADDR    0x70

/* Downstream channel assignment (schematic net names) */
#define TCA9548A_CH_PPG1     0   /* SDA_PPG1 / SCL_PPG1 */
#define TCA9548A_CH_PPG2     1   /* SDA_PPG2 / SCL_PPG2 */
#define TCA9548A_CH_PPG3     2   /* SDA_PPG3 / SCL_PPG3 */
#define TCA9548A_CH_TH       3   /* SDA_TH   / SCL_TH — temp/humidity */

/**
 * @brief Initialize the TCA9548A I2C multiplexer.
 *
 * Releases the active-low reset line (P1.00), probes the device at
 * 0x70, and disables all downstream channels.
 *
 * @return 0 on success, negative errno on failure.
 */
int drv_tca9548a_init(void);

/**
 * @brief Enable exactly one downstream channel (0-7).
 *
 * @param channel  Channel index 0-7 (see TCA9548A_CH_* macros).
 * @return 0 on success, negative errno on failure.
 */
int drv_tca9548a_select(uint8_t channel);

/**
 * @brief Disable all downstream channels.
 *
 * @return 0 on success, negative errno on failure.
 */
int drv_tca9548a_deselect_all(void);

/**
 * @brief Pulse the hardware reset line (active low, P1.00).
 *
 * @return 0 on success, negative errno on failure.
 */
int drv_tca9548a_reset(void);

#endif /* DRIVER_TCA9548A_H */
