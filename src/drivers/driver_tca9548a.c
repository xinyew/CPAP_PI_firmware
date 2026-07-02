/*
 * TCA9548A 8-channel I2C multiplexer driver.
 *
 * Sits on i2c0 (SDA = P0.08, SCL = P0.12) at address 0x70
 * (A0 = A1 = A2 = GND). Hardware reset on P1.00, active low.
 *
 * The control register is a single byte: bit N enables downstream
 * channel N. Writing 0x00 disconnects all channels.
 *
 * Channel map (this board):
 *   ch0 — PPG sensor 1
 *   ch1 — PPG sensor 2
 *   ch2 — PPG sensor 3
 *   ch3 — Temp/Humidity sensor
 *   ch4-7 — unused
 */

#include "driver_tca9548a.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(tca9548a, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------- */
/*  Hardware handles                                                          */
/* -------------------------------------------------------------------------- */

#define TCA9548A_I2C  DEVICE_DT_GET(DT_NODELABEL(i2c0))

static const struct device *i2c_dev = TCA9548A_I2C;

static const struct gpio_dt_spec reset_gpio =
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), tca_reset_gpios);

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

int drv_tca9548a_init(void)
{
    int ret;

    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C0 not ready");
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&reset_gpio)) {
        LOG_ERR("TCA_RST GPIO not ready");
        return -ENODEV;
    }

    /* Release reset (active low — inactive means line HIGH) */
    ret = gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("TCA_RST configure failed: %d", ret);
        return ret;
    }
    k_usleep(10);  /* t_RST recovery is < 1 us; give it margin */

    /* Probe: disable all channels (write 0x00) */
    ret = drv_tca9548a_deselect_all();
    if (ret < 0) {
        LOG_ERR("TCA9548A @0x%02X not responding: %d", TCA9548A_I2C_ADDR, ret);
        return ret;
    }

    LOG_INF("TCA9548A found at 0x%02X", TCA9548A_I2C_ADDR);
    return 0;
}

int drv_tca9548a_select(uint8_t channel)
{
    if (channel > 7) {
        return -EINVAL;
    }

    uint8_t ctrl = BIT(channel);
    int ret = i2c_write(i2c_dev, &ctrl, 1, TCA9548A_I2C_ADDR);
    if (ret < 0) {
        LOG_ERR("Select channel %u failed: %d", channel, ret);
    }
    return ret;
}

int drv_tca9548a_deselect_all(void)
{
    uint8_t ctrl = 0x00;
    return i2c_write(i2c_dev, &ctrl, 1, TCA9548A_I2C_ADDR);
}

int drv_tca9548a_reset(void)
{
    int ret = gpio_pin_set_dt(&reset_gpio, 1);  /* assert (LOW) */
    if (ret < 0) {
        return ret;
    }
    k_usleep(10);
    ret = gpio_pin_set_dt(&reset_gpio, 0);      /* release */
    k_usleep(10);
    return ret;
}
