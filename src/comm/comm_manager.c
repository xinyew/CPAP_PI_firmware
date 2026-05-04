#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>

#include "comm/comm_manager.h"
#include "comm/ble_manager.h"

LOG_MODULE_REGISTER(comm_manager, LOG_LEVEL_INF);

static comm_interface_type_t active_interface = COMM_IF_SERIAL;

/* --- Serial/UART Interface --- */
static int serial_init(void) { return 0; }
static int serial_send(const char *data, size_t len) 
{ 
    printf("%s", data);
    return 0; 
}

/* --- BLE/NUS Interface --- */
static int ble_init(void) { return ble_manager_init(); }
static int ble_send(const char *data, size_t len)
{
    return ble_manager_send(data, len);
}

static comm_interface_t interfaces[] = {
    [COMM_IF_SERIAL] = { .name = "Serial", .init = serial_init, .send_data = serial_send },
    [COMM_IF_BLE]    = { .name = "BLE (NUS)", .init = ble_init, .send_data = ble_send },
};

int comm_manager_init(void)
{
    int err = 0;
    for (int i = 0; i < COMM_IF_COUNT; i++) {
        if (interfaces[i].init) {
            int ret = interfaces[i].init();
            if (ret < 0) {
                LOG_ERR("Failed to initialize interface %s (%d)", interfaces[i].name, ret);
                err = ret;
            }
        }
    }
    return err;
}

void comm_manager_set_interface(comm_interface_type_t type)
{
    if (type < COMM_IF_COUNT) {
        active_interface = type;
        LOG_INF("Comm interface switched to: %s", interfaces[type].name);
    }
}

comm_interface_type_t comm_manager_get_interface(void)
{
    return active_interface;
}

void comm_manager_broadcast(const char *data, size_t len)
{
    if (interfaces[active_interface].send_data) {
        interfaces[active_interface].send_data(data, len);
    }
}
