#ifndef BLE_INTERFACE_H
#define BLE_INTERFACE_H

#include <zephyr/kernel.h>

/**
 * @brief Initialise Bluetooth — GATT server, advertising, sample-rate control.
 *
 * After this call the board advertises as "CPAP_PI_Body" and accepts
 * Write-Without-Response to a custom characteristic that sets the sensor
 * sampling interval (uint16_t, little-endian, milliseconds).
 *
 * @return 0 on success, negative errno on failure.
 */
int ble_interface_init(void);

/**
 * @brief Current sensor sampling interval in milliseconds.
 *
 * Defaults to 200 ms; updated over BLE.
 */
uint16_t ble_interface_get_sample_interval_ms(void);

#endif /* BLE_INTERFACE_H */
