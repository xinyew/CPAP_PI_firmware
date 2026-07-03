#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <zephyr/kernel.h>

typedef void (*ble_rx_cb_t)(const uint8_t *data, uint16_t len);

/**
 * @brief Initialise Bluetooth: NUS service + connectable advertising.
 *
 * Advertises as CONFIG_BT_DEVICE_NAME ("CPAP_PI_Control"); the web
 * portal matches on the "CPAP" name prefix. LED2 indicates a live
 * connection. Re-advertises automatically on disconnect.
 *
 * @param rx_cb  Called for data written to the NUS RX characteristic.
 * @return 0 on success, negative errno on failure.
 */
int ble_manager_init(ble_rx_cb_t rx_cb);

/** @brief True when connected AND the peer enabled NUS notifications. */
bool ble_manager_can_send(void);

/**
 * @brief Send one NUS notification (whole buffer = one frame).
 *
 * @return 0 on success, negative errno otherwise.
 */
int ble_manager_send(const uint8_t *data, uint16_t len);

#endif /* BLE_MANAGER_H */
