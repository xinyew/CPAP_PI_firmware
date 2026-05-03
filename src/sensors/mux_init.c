#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>

#include "sensors/mux_init.h"

LOG_MODULE_REGISTER(mux_init, LOG_LEVEL_INF);

/* Get GPIOs from the zephyr,user node */
static const struct gpio_dt_spec mux_a0 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), mux_a0_gpios);
static const struct gpio_dt_spec mux_a1 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), mux_a1_gpios);
static const struct gpio_dt_spec mux_a2 = GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), mux_a2_gpios);

int mux_init_hardware(void)
{
    int ret;

    if (!gpio_is_ready_dt(&mux_a0) || !gpio_is_ready_dt(&mux_a1) || !gpio_is_ready_dt(&mux_a2)) {
        LOG_ERR("MUX address GPIOs not ready");
        return -ENODEV;
    }

    /* Configure pins as output and set them LOW to achieve 0x70 address */
    ret = gpio_pin_configure_dt(&mux_a0, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure A0");
        return ret;
    }

    ret = gpio_pin_configure_dt(&mux_a1, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure A1");
        return ret;
    }

    ret = gpio_pin_configure_dt(&mux_a2, GPIO_OUTPUT_INACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure A2");
        return ret;
    }

    LOG_INF("MUX hardware pins initialized. Address set to 0x70.");
    return 0;
}

/* Run this at POST_KERNEL level, priority 40 to ensure it happens BEFORE I2C (priority ~50) and sensors (~90) */
SYS_INIT(mux_init_hardware, POST_KERNEL, 40);
