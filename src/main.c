/*
 * CPAP PI Sensor Body — PPG + FSR Sensing Firmware
 * Custom nRF52840 board (cpap_pi_body/nrf52840, Raytac MDBT50Q-P1M)
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include "drivers/driver_tca9548a.h"
#include "drivers/driver_fsr.h"
#include "drivers/driver_led.h"
#include "interface/ble_interface.h"

LOG_MODULE_REGISTER(cpap_pi_main, LOG_LEVEL_DBG);

int main(void)
{
    printk("\n=== CPAP PI Sensor Body Boot ===\n");

    /* Status LEDs (LED1 = P1.08, LED2 = P1.09, active low) */
    if (drv_led_init() < 0) {
        LOG_ERR("Failed to init LEDs");
    }

    /* TCA9548A I2C mux — release reset, probe @0x70 */
    if (drv_tca9548a_init() < 0) {
        LOG_ERR("Failed to init TCA9548A");
    }

    /* SAADC — FSR channels FF1-FF3 + VREF */
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
