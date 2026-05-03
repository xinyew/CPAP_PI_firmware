#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "comm/comm_manager.h"

LOG_MODULE_REGISTER(comm_manager, LOG_LEVEL_INF);

static int serial_swo_init(void)
{
    return 0;
}

static int serial_swo_send(const char *data, size_t len)
{
    printk("%s\n", data);
    return 0;
}

static comm_interface_t default_interfaces[] = {
    {
        .name = "Serial/SWO Logger",
        .init = serial_swo_init,
        .send_data = serial_swo_send,
    }
};

#define NUM_INTERFACES (sizeof(default_interfaces) / sizeof(default_interfaces[0]))

int comm_manager_init(void)
{
    int err = 0;
    for (int i = 0; i < NUM_INTERFACES; i++) {
        if (default_interfaces[i].init) {
            int ret = default_interfaces[i].init();
            if (ret < 0) {
                LOG_ERR("Failed to initialize comm interface: %s", default_interfaces[i].name);
                err = ret;
            } else {
                LOG_INF("Comm interface initialized: %s", default_interfaces[i].name);
            }
        }
    }
    return err;
}

void comm_manager_broadcast(const char *data, size_t len)
{
    for (int i = 0; i < NUM_INTERFACES; i++) {
        if (default_interfaces[i].send_data) {
            default_interfaces[i].send_data(data, len);
        }
    }
}
