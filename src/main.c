#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>

#include "comm/comm_manager.h"
#include "sensors/sensor_manager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static void scan_i2c(void)
{
    const struct device *i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C0 not ready for diagnostic scan");
        return;
    }

    LOG_INF("--- DIAGNOSTIC: Scanning I2C bus ---");
    for (uint8_t i = 8; i < 120; i++) {
        uint8_t dst = 0;
        int err = i2c_write(i2c_dev, &dst, 0, i);
        if (err == 0) {
            LOG_INF("Found I2C device at address: 0x%02X", i);
        }
    }
    
    /* Try to force open MUX channel 0 */
    LOG_INF("Attempting to force open MUX Channel 0 (writing 0x01 to 0x70)...");
    uint8_t ch_select = 0x01;
    if (i2c_write(i2c_dev, &ch_select, 1, 0x70) == 0) {
        LOG_INF("MUX write successful. Scanning again...");
        for (uint8_t i = 8; i < 120; i++) {
            uint8_t dst = 0;
            int err = i2c_write(i2c_dev, &dst, 0, i);
            if (err == 0) {
                LOG_INF("Found I2C device at address: 0x%02X", i);
            }
        }
    } else {
        LOG_ERR("MUX (0x70) did NOT acknowledge write!");
    }
    LOG_INF("--- DIAGNOSTIC COMPLETE ---");
}

int main(void)
{
    int ret;

    LOG_INF("Starting CPAP Pressure Injury Sensing Application (Phase 1)");

    /* Initialize Communication Interfaces */
    ret = comm_manager_init();
    if (ret < 0) {
        LOG_ERR("Failed to initialize communication manager");
    }

    /* Give everything 100ms to settle */
    k_msleep(100);

    /* Run I2C Diagnostic Scan */
    scan_i2c();

    /* Initialize Sensors */
    ret = sensor_manager_init();
    if (ret < 0) {
        LOG_WRN("Some sensors failed to initialize");
    }

    LOG_INF("Initialization Complete. Entering unified 60Hz reporting loop.");

    uint32_t last_report_time = k_uptime_get_32();
    uint32_t report_interval = 16; // ~60Hz (16ms)

    while (1) {
        // Poll sensors in the background at their individual frequencies
        sensor_manager_poll();
        
        // Report unified JSON at exactly 60Hz
        uint32_t now = k_uptime_get_32();
        if ((now - last_report_time) >= report_interval) {
            sensor_manager_report();
            last_report_time = now;
        }

        // Sleep 2ms to prevent CPU hogging and allow other threads to run
        k_msleep(2);
    }

    return 0;
}
