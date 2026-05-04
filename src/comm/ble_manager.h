#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <zephyr/types.h>
#include <stddef.h>

int ble_manager_init(void);
int ble_manager_send(const char *data, size_t len);

#endif /* BLE_MANAGER_H */
