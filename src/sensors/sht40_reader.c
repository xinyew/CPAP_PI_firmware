/*
 * SHT40 temp/humidity reader — thin wrapper over the in-tree sht4x
 * driver (TCA9548A channel 3, high repeatability). Polled at 1 Hz;
 * higher duty would self-heat the sensor.
 */

#include "sht40_reader.h"

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(sht40_reader, LOG_LEVEL_INF);

static const struct device *const sht_dev = DEVICE_DT_GET(DT_NODELABEL(sht40));

int sht40_reader_init(void)
{
    if (!device_is_ready(sht_dev)) {
        LOG_WRN("SHT40 absent");
        return -ENODEV;
    }

    LOG_INF("SHT40 online (1 Hz, high repeatability)");
    return 0;
}

int sht40_reader_read(int32_t *temp_c100, int32_t *rh_x100)
{
    if (!device_is_ready(sht_dev)) {
        return -ENODEV;
    }

    int ret = sensor_sample_fetch(sht_dev);

    if (ret < 0) {
        return ret;
    }

    struct sensor_value t, rh;

    ret = sensor_channel_get(sht_dev, SENSOR_CHAN_AMBIENT_TEMP, &t);
    if (ret < 0) {
        return ret;
    }
    ret = sensor_channel_get(sht_dev, SENSOR_CHAN_HUMIDITY, &rh);
    if (ret < 0) {
        return ret;
    }

    *temp_c100 = t.val1 * 100 + t.val2 / 10000;
    *rh_x100 = rh.val1 * 100 + rh.val2 / 10000;
    return 0;
}
