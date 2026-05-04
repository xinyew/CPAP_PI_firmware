#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include <dk_buttons_and_leds.h>

#include "comm/comm_manager.h"
#include "sensors/sensor_manager.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_WRN);

static void update_led_indication(comm_interface_type_t type)
{
    /* Assuming LED1, LED2, LED3 for R/B/G indication on DK */
    dk_set_leds(0); // Turn off all
    switch (type) {
        case COMM_IF_SERIAL: dk_set_led(DK_LED1, 1); break;
        case COMM_IF_BLE:    dk_set_led(DK_LED2, 1); break;
        default: break;
    }
}

static void button_handler(uint32_t button_state, uint32_t has_changed)
{
    if (has_changed & button_state & DK_BTN1_MSK) {
        comm_interface_type_t current = comm_manager_get_interface();
        comm_interface_type_t next = (current + 1) % COMM_IF_COUNT;
        comm_manager_set_interface(next);
        update_led_indication(next);
    }
}

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

    /* Initialize Buttons and LEDs */
    ret = dk_buttons_init(button_handler);
    if (ret) {
        LOG_ERR("Failed to initialize buttons (err %d)", ret);
    }
    ret = dk_leds_init();
    if (ret) {
        LOG_ERR("Failed to initialize LEDs (err %d)", ret);
    }
    update_led_indication(COMM_IF_SERIAL);

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

    LOG_INF("Initialization Complete. Starting 60Hz Timer-based reporting.");

    while (1) {
        // Poll sensors as fast as possible to fill the cache
        sensor_manager_poll();
        
        // Report unified JSON at exactly 60Hz (driven by k_msleep for now, but tighter)
        sensor_manager_report();
        
        // 1000ms / 60 = 16.66ms. k_msleep(16) is the closest.
        k_msleep(16);
    }

    return 0;
}
