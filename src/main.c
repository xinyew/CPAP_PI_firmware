/*
 * CPAP PI Sensor Control — PPG + FSR + Pressure Sensing Firmware
 * Custom nRF52840 board (cpap_pi_control/nrf52840, Raytac MDBT50Q-P1M)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "drivers/bus_diag.h"
#include "drivers/driver_led.h"
#include "sensors/sensor_manager.h"
#include "interface/ble_interface.h"

LOG_MODULE_REGISTER(cpap_pi_main, LOG_LEVEL_DBG);

int main(void)
{
    printk("\n=== CPAP PI Sensor Control Boot ===\n");

    /* Status LEDs (LED1 = P1.08, LED2 = P1.09, active low) */
    if (drv_led_init() < 0) {
        LOG_ERR("Failed to init LEDs");
    }

    /* Bring-up diagnostics: TCA9548A channels and MS5611 PROM CRCs */
    int i2c_ok = bus_diag_scan_mux();
    int baro_ok = bus_diag_ms5611_check();
    printk("Bus diag: %d/4 I2C sensors, %d/6 baros OK\n", i2c_ok, baro_ok);

    /* BLE GATT server — remote sampling-interval control */
    if (ble_interface_init() < 0) {
        LOG_ERR("Failed to init BLE");
    }

    /* Sampling thread: PPG + FSR 100 Hz, baro 25 Hz, SHT40 1 Hz */
    sensor_manager_start();

    while (1) {
        drv_led_toggle(LED_1);  /* heartbeat */
        k_msleep(500);
    }
}
