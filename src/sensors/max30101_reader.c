#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "sensors/max30101_reader.h"
#include "comm/comm_manager.h"

LOG_MODULE_REGISTER(max30101_reader, LOG_LEVEL_INF);

static const struct device *max30101_dev = DEVICE_DT_GET_ANY(maxim_max30101);

static void max30101_trigger_handler(const struct device *dev, const struct sensor_trigger *trig)
{
    struct sensor_value red, ir, green;
    char buf[128];

    if (sensor_sample_fetch(dev) < 0) {
        LOG_ERR("Failed to fetch MAX30101 sample in trigger");
        return;
    }

    sensor_channel_get(dev, SENSOR_CHAN_RED, &red);
    sensor_channel_get(dev, SENSOR_CHAN_IR, &ir);
    sensor_channel_get(dev, SENSOR_CHAN_GREEN, &green);

    snprintf(buf, sizeof(buf), "MAX30101 [IRQ]: Red %d, IR %d, Green %d",
             red.val1, ir.val1, green.val1);
    
    comm_manager_broadcast(buf, strlen(buf));
}

static int max30101_init(void)
{
    if (!device_is_ready(max30101_dev)) {
        LOG_ERR("MAX30101 device not ready");
        return -ENODEV;
    }

    struct sensor_trigger trig = {
        .type = SENSOR_TRIG_DATA_READY,
        .chan = SENSOR_CHAN_ALL,
    };

    if (sensor_trigger_set(max30101_dev, &trig, max30101_trigger_handler) < 0) {
        LOG_ERR("Could not configure MAX30101 trigger");
        return -EIO;
    }

    LOG_INF("MAX30101 trigger configured successfully.");
    return 0;
}

sensor_interface_t max30101_sensor = {
    .name = "MAX30101",
    .init = max30101_init,
    .read = NULL,
    .get_interval_ms = NULL,
};
