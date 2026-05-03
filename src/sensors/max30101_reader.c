#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "sensors/max30101_reader.h"
#include "comm/comm_manager.h"

LOG_MODULE_REGISTER(max30101_reader, LOG_LEVEL_INF);

static const struct device *max30101_dev = DEVICE_DT_GET_ANY(maxim_max30101);

static int max30101_init(void)
{
    if (!device_is_ready(max30101_dev)) {
        LOG_ERR("MAX30101 device not ready");
        return -ENODEV;
    }
    return 0;
}

static int max30101_read(void)
{
    struct sensor_value red, ir, green;
    int ret;
    char buf[128];

    ret = sensor_sample_fetch(max30101_dev);
    if (ret) {
        LOG_ERR("Failed to fetch MAX30101 sample (err: %d)", ret);
        return ret;
    }

    sensor_channel_get(max30101_dev, SENSOR_CHAN_RED, &red);
    sensor_channel_get(max30101_dev, SENSOR_CHAN_IR, &ir);
    sensor_channel_get(max30101_dev, SENSOR_CHAN_GREEN, &green);

    snprintf(buf, sizeof(buf), "MAX30101: Red %d, IR %d, Green %d",
             red.val1, ir.val1, green.val1);
    
    comm_manager_broadcast(buf, strlen(buf));
    return 0;
}

static uint32_t max30101_get_interval(void)
{
    return 1000; /* 1 second */
}

sensor_interface_t max30101_sensor = {
    .name = "MAX30101",
    .init = max30101_init,
    .read = max30101_read,
    .get_interval_ms = max30101_get_interval,
};
