#ifndef DRIVER_MS5611_H
#define DRIVER_MS5611_H

#include <zephyr/kernel.h>

#define MS5611_COUNT  6

/**
 * @brief Initialize all six MS5611 barometers.
 *
 * Resets each sensor, reads and CRC-checks its calibration PROM, and
 * seeds the temperature compensation with one blocking D2 conversion.
 * Sensors that fail (e.g. baro1 with its broken CS line) are marked
 * absent and skipped by all later calls.
 *
 * @return Number of responsive sensors (0-6).
 */
int drv_ms5611_init(void);

/** @brief Bitmask of responsive sensors (bit N = baro N+1). */
uint8_t drv_ms5611_ok_mask(void);

/**
 * @brief Start a conversion on all responsive sensors concurrently.
 *
 * OSR 2048 — max conversion time 4.6 ms; call the matching
 * drv_ms5611_finish_conv() no sooner than 5 ms later.
 *
 * @param temperature  true = D2 (temperature), false = D1 (pressure).
 * @return 0 on success, negative errno on first failure.
 */
int drv_ms5611_start_conv(bool temperature);

/**
 * @brief Read out the previously started conversion on all sensors.
 *
 * Temperature readouts update the per-sensor dT used to compensate
 * pressure; pressure readouts update the compensated pressure value.
 *
 * @return 0 on success, negative errno on first failure.
 */
int drv_ms5611_finish_conv(void);

/**
 * @brief Latest compensated values for sensor idx (0-5).
 *
 * @param press_pa   Pressure in Pa (= 0.01 mbar resolution).
 * @param temp_c100  Temperature in 0.01 degC.
 */
void drv_ms5611_get(int idx, int32_t *press_pa, int32_t *temp_c100);

#endif /* DRIVER_MS5611_H */
