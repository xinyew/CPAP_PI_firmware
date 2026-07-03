/**
 * Copyright (c) 2015 - present LibDriver All rights reserved
 * 
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 * @file      driver_ms5611_interface.c
 * @brief     driver ms5611 interface source file
 * @version   1.0.0
 * @author    Shifeng Li
 * @date      2024-03-31
 */

#include "driver_ms5611_interface.h"
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <stdarg.h>

LOG_MODULE_REGISTER(ms5611_interface, LOG_LEVEL_WRN);

/* Retrieve I2C multiplexer channel 1 device node */
static const struct device *ms5611_i2c_dev = DEVICE_DT_GET(DT_NODELABEL(mux_i2c1));

/**
 * @brief  interface iic bus init
 * @return status code
 *         - 0 success
 *         - 1 iic init failed
 */
uint8_t ms5611_interface_iic_init(void)
{
    if (!device_is_ready(ms5611_i2c_dev)) {
        LOG_ERR("I2C mux channel 1 device not ready for MS5611!");
        return 1;
    }
    return 0;
}

/**
 * @brief  interface iic bus deinit
 * @return status code
 *         - 0 success
 *         - 1 iic deinit failed
 */
uint8_t ms5611_interface_iic_deinit(void)
{
    return 0;
}

/**
 * @brief      interface iic bus read
 * @param[in]  addr iic device write address (8-bit)
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 */
uint8_t ms5611_interface_iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    /* Libdriver passes the 8-bit I2C write address (0xEE or 0xEC) */
    /* Zephyr I2C functions expect a 7-bit address (0x77 or 0x76) */
    uint8_t dev_addr = addr >> 1;
    
    int err = i2c_write_read(ms5611_i2c_dev, dev_addr, &reg, 1, buf, len);
    if (err) {
        LOG_ERR("MS5611 I2C read failed: reg=0x%02X, err=%d", reg, err);
        return 1;
    }
    return 0;
}

/**
 * @brief     interface iic bus write
 * @param[in] addr iic device write address (8-bit)
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 */
uint8_t ms5611_interface_iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t dev_addr = addr >> 1;
    
    if (len == 0) {
        int err = i2c_write(ms5611_i2c_dev, &reg, 1, dev_addr);
        if (err) {
            LOG_ERR("MS5611 I2C single-byte write failed: reg=0x%02X, err=%d", reg, err);
            return 1;
        }
        return 0;
    } else {
        uint8_t tx_buf[len + 1];
        tx_buf[0] = reg;
        memcpy(&tx_buf[1], buf, len);
        
        int err = i2c_write(ms5611_i2c_dev, tx_buf, len + 1, dev_addr);
        if (err) {
            LOG_ERR("MS5611 I2C multi-byte write failed: reg=0x%02X, err=%d", reg, err);
            return 1;
        }
        return 0;
    }
}

/**
 * @brief  interface spi bus init
 * @return status code
 *         - 0 success
 *         - 1 spi init failed
 */
uint8_t ms5611_interface_spi_init(void)
{
    return 0;
}

/**
 * @brief  interface spi bus deinit
 * @return status code
 *         - 0 success
 *         - 1 spi deinit failed
 */
uint8_t ms5611_interface_spi_deinit(void)
{   
    return 0;
}

/**
 * @brief      interface spi bus read
 * @param[in]  reg register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 */
uint8_t ms5611_interface_spi_read(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return 0;
}

/**
 * @brief     interface spi bus write
 * @param[in] reg register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 */
uint8_t ms5611_interface_spi_write(uint8_t reg, uint8_t *buf, uint16_t len)
{
    return 0;
}

/**
 * @brief     interface delay ms
 * @param[in] ms time
 */
void ms5611_interface_delay_ms(uint32_t ms)
{
    k_msleep(ms);
}

/**
 * @brief     interface print format data
 * @param[in] fmt format data
 */
void ms5611_interface_debug_print(const char *const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
