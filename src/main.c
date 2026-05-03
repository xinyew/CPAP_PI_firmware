#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "comm/comm_manager.h"
#include "sensors/sensor_manager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

int main(void)
{
    int ret;

    LOG_INF("Starting CPAP Pressure Injury Sensing Application (Phase 1)");

    /* Initialize Communication Interfaces */
    ret = comm_manager_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize communication manager");
    }

    /* Initialize Sensors */
    ret = sensor_manager_init();
    if (ret < 0) {
        LOG_WRN("Some sensors failed to initialize");
    }

    LOG_INF("Initialization Complete. Entering main loop.");

    while (1) {
        sensor_manager_poll();
        k_msleep(100);
    }

    return 0;
}
