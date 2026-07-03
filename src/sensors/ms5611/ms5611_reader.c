#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

#include "sensors/ms5611/ms5611_reader.h"
#include "driver_ms5611_basic.h"

LOG_MODULE_REGISTER(ms5611_reader, LOG_LEVEL_INF);

static int ms5611_reader_init(void)
{
    LOG_INF("Initializing MS5611 Pressure Sensor...");

    /* Auto-detection fallback:
     * GY-63 can have I2C address 0x77 (CSB high/floating) or 0x76 (CSB low).
     * Libdriver uses 8-bit representation: CSB_0 (0xEE) -> 0x77, CSB_1 (0xEC) -> 0x76.
     */
    uint8_t res = ms5611_basic_init(MS5611_INTERFACE_IIC, MS5611_ADDRESS_CSB_0);
    if (res == 0) {
        LOG_INF("MS5611 successfully initialized at CSB_0 (0x77/0xEE).");
        return 0;
    }

    LOG_WRN("MS5611 not found at CSB_0 (0x77), trying CSB_1 (0x76/0xEC)...");
    ms5611_basic_deinit();

    res = ms5611_basic_init(MS5611_INTERFACE_IIC, MS5611_ADDRESS_CSB_1);
    if (res == 0) {
        LOG_INF("MS5611 successfully initialized at CSB_1 (0x76/0xEC).");
        return 0;
    }

    LOG_ERR("MS5611 Pressure Sensor not detected on I2C bus!");
    return -ENODEV;
}

static int ms5611_reader_read(void)
{
    float temp_c = 0.0f;
    float pressure_mbar = 0.0f;

    uint8_t res = ms5611_basic_read(&temp_c, &pressure_mbar);
    if (res == 0) {
        current_sensor_data.pressure_mbar = pressure_mbar;
        current_sensor_data.ms5611_temp_c = temp_c;
        return 0;
    } else {
        LOG_ERR("Failed to read from MS5611 sensor");
        return -EIO;
    }
}

static uint32_t ms5611_reader_get_interval(void)
{
    /* 10Hz reading frequency (100ms interval) */
    return 100;
}

sensor_interface_t ms5611_sensor = {
    .name = "MS5611",
    .init = ms5611_reader_init,
    .read = ms5611_reader_read,
    .get_interval_ms = ms5611_reader_get_interval,
};
