#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "sensors/sensor_manager.h"
#include "sensors/max30101/max30101_reader.h"

LOG_MODULE_REGISTER(sensor_manager, LOG_LEVEL_INF);

static sensor_interface_t *sensors[] = {
    &max30101_sensor,
};

#define NUM_SENSORS (sizeof(sensors) / sizeof(sensors[0]))

int sensor_manager_init(void)
{
    int err = 0;
    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensors[i]->init) {
            int ret = sensors[i]->init();
            if (ret < 0) {
                LOG_ERR("Failed to initialize sensor: %s (err: %d)", sensors[i]->name, ret);
                err = ret;
            } else {
                LOG_INF("Sensor initialized successfully: %s", sensors[i]->name);
            }
        }
        sensors[i]->last_read_time_ms = k_uptime_get_32();
    }
    return err;
}

void sensor_manager_poll(void)
{
    uint32_t now = k_uptime_get_32();

    for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensors[i]->read && sensors[i]->get_interval_ms) {
            uint32_t interval = sensors[i]->get_interval_ms();
            if ((now - sensors[i]->last_read_time_ms) >= interval) {
                sensors[i]->read();
                sensors[i]->last_read_time_ms = now;
            }
        }
    }
}
