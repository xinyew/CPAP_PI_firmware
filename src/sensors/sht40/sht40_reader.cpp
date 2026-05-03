#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

#include "sensors/sht40/sht40_reader.h"
#include "comm/comm_manager.h"
#include "SHT40.h"

LOG_MODULE_REGISTER(sht40_reader, LOG_LEVEL_INF);

SHT40 sht40;

extern "C" {

static int sht40_init(void)
{
    LOG_INF("Initializing SHT40 using MR01Right Library...");

    // Initialize sensor on Wire1 (MUX Channel 1)
    if (!sht40.begin(&Wire1, 0x44)) {
        LOG_ERR("SHT40 was not found. Please check wiring/power.");
        return -ENODEV;
    }

    LOG_INF("SHT40 successfully configured.");
    return 0;
}

static int sht40_read(void)
{
    char buf[128];
    float temp, hum;

    if (sht40.measure(temp, hum)) {
        snprintf(buf, sizeof(buf), "SHT40: Temp %.2f C, Hum %.2f %%", (double)temp, (double)hum);
        comm_manager_broadcast(buf, strlen(buf));
        return 0;
    } else {
        LOG_ERR("Failed to measure SHT40");
        return -EIO;
    }
}

static uint32_t sht40_get_interval(void)
{
    // Poll every 2 seconds
    return 2000;
}

sensor_interface_t sht40_sensor = {
    .name = "SHT40",
    .init = sht40_init,
    .read = sht40_read,
    .get_interval_ms = sht40_get_interval,
};

} // extern "C"
