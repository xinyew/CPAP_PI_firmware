/*
 * CPAP PI Sensor Control — PPG + FSR + Pressure Sensing Firmware
 * Custom nRF52840 board (cpap_pi_control/nrf52840, Raytac MDBT50Q-P1M)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "drivers/bus_diag.h"
#include "drivers/driver_fsr.h"
#include "drivers/driver_led.h"
#include "interface/ble_interface.h"

LOG_MODULE_REGISTER(cpap_pi_main, LOG_LEVEL_DBG);

int main(void)
{
    printk("\n=== CPAP PI Sensor Control Boot ===\n");

    /* Status LEDs (LED1 = P1.08, LED2 = P1.09, active low) */
    if (drv_led_init() < 0) {
        LOG_ERR("Failed to init LEDs");
    }

    /* Bring-up diagnostics: TCA9548A channels (DT mux driver) and the
     * six MS5611 barometers on the mask flex.
     */
    int i2c_ok = bus_diag_scan_mux();
    int baro_ok = bus_diag_ms5611_check();
    printk("Bus diag: %d/4 I2C sensors, %d/6 baros OK\n", i2c_ok, baro_ok);

    /* SAADC — FSR channels FF1-FF3 + VREF (from devicetree) */
    if (drv_fsr_init() < 0) {
        LOG_ERR("Failed to init FSR ADC");
    }

    /* BLE GATT server — remote sampling-interval control */
    if (ble_interface_init() < 0) {
        LOG_ERR("Failed to init BLE");
    }

    uint32_t tick = 0;
    while (1) {
        struct fsr_data fsr;
        if (drv_fsr_read(&fsr) == 0) {
            printk("[%5u] FF1: %4d mV  FF2: %4d mV  FF3: %4d mV  |  VREF: %4d mV\n",
                   tick,
                   fsr.ff_mv[0], fsr.ff_mv[1], fsr.ff_mv[2],
                   fsr.vref_mv);
        }

        drv_led_toggle(LED_1);  /* heartbeat */

        tick++;
        k_msleep(ble_interface_get_sample_interval_ms());
    }
}
