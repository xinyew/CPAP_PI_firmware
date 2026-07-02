/*
 * Bring-up diagnostics for the mask flex sensor buses.
 *
 * - I2C: scans each TCA9548A downstream channel (DT mux driver provides
 *   one bus device per channel) for the expected sensors.
 * - SPI: resets each MS5611 and validates its calibration PROM CRC-4
 *   (AN520 algorithm) using raw SPI transfers.
 */

#include "bus_diag.h"

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(bus_diag, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------- */
/*  I2C mux channel scan                                                      */
/* -------------------------------------------------------------------------- */

static const struct device *const mux_ch[] = {
    DEVICE_DT_GET(DT_NODELABEL(mux_i2c0)),
    DEVICE_DT_GET(DT_NODELABEL(mux_i2c1)),
    DEVICE_DT_GET(DT_NODELABEL(mux_i2c2)),
    DEVICE_DT_GET(DT_NODELABEL(mux_i2c3)),
};

static const uint8_t expected_addr[] = { 0x57, 0x57, 0x57, 0x44 };

static bool probe_addr(const struct device *bus, uint8_t addr)
{
    /* Address-only write; fall back to a 1-byte read for devices that
     * NAK zero-length writes on this controller.
     */
    if (i2c_write(bus, NULL, 0, addr) == 0) {
        return true;
    }
    uint8_t b;
    return i2c_read(bus, &b, 1, addr) == 0;
}

int bus_diag_scan_mux(void)
{
    int found_expected = 0;

    for (size_t ch = 0; ch < ARRAY_SIZE(mux_ch); ch++) {
        if (!device_is_ready(mux_ch[ch])) {
            LOG_ERR("mux ch%u bus not ready", (unsigned)ch);
            continue;
        }

        int hits = 0;
        for (uint8_t addr = 0x08; addr < 0x78; addr++) {
            if (probe_addr(mux_ch[ch], addr)) {
                LOG_INF("mux ch%u: found 0x%02X%s", (unsigned)ch, addr,
                        addr == expected_addr[ch] ? " (expected)" : "");
                hits++;
                if (addr == expected_addr[ch]) {
                    found_expected++;
                }
            }
        }
        if (hits == 0) {
            LOG_WRN("mux ch%u: no devices", (unsigned)ch);
        }
    }

    LOG_INF("mux scan: %d/4 expected sensors present", found_expected);
    return found_expected;
}

/* -------------------------------------------------------------------------- */
/*  MS5611 PROM CRC check                                                     */
/* -------------------------------------------------------------------------- */

#define MS5611_SPI_OP  (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

static const struct spi_dt_spec baros[] = {
    SPI_DT_SPEC_GET(DT_NODELABEL(baro1), MS5611_SPI_OP),
    SPI_DT_SPEC_GET(DT_NODELABEL(baro2), MS5611_SPI_OP),
    SPI_DT_SPEC_GET(DT_NODELABEL(baro3), MS5611_SPI_OP),
    SPI_DT_SPEC_GET(DT_NODELABEL(baro4), MS5611_SPI_OP),
    SPI_DT_SPEC_GET(DT_NODELABEL(baro5), MS5611_SPI_OP),
    SPI_DT_SPEC_GET(DT_NODELABEL(baro6), MS5611_SPI_OP),
};

#define MS5611_CMD_RESET      0x1E
#define MS5611_CMD_PROM_READ  0xA0  /* + 2*word_index */

static int ms5611_cmd(const struct spi_dt_spec *spec, uint8_t cmd)
{
    const struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
    const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    return spi_write_dt(spec, &tx);
}

static int ms5611_prom_word(const struct spi_dt_spec *spec, uint8_t idx,
                            uint16_t *word)
{
    uint8_t tx_data[3] = { MS5611_CMD_PROM_READ + 2 * idx, 0, 0 };
    uint8_t rx_data[3];
    const struct spi_buf tx_buf = { .buf = tx_data, .len = sizeof(tx_data) };
    const struct spi_buf rx_buf = { .buf = rx_data, .len = sizeof(rx_data) };
    const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
    const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

    int ret = spi_transceive_dt(spec, &tx, &rx);
    if (ret < 0) {
        return ret;
    }
    *word = ((uint16_t)rx_data[1] << 8) | rx_data[2];
    return 0;
}

/* CRC-4 over the calibration PROM, per TE application note AN520 */
static uint8_t ms5611_crc4(uint16_t prom[8])
{
    uint16_t n_rem = 0;
    uint16_t crc_save = prom[7];

    prom[7] &= 0xFF00; /* CRC nibble is excluded from the calculation */

    for (int cnt = 0; cnt < 16; cnt++) {
        if (cnt & 1) {
            n_rem ^= prom[cnt >> 1] & 0x00FF;
        } else {
            n_rem ^= prom[cnt >> 1] >> 8;
        }
        for (int bit = 8; bit > 0; bit--) {
            if (n_rem & 0x8000) {
                n_rem = (n_rem << 1) ^ 0x3000;
            } else {
                n_rem <<= 1;
            }
        }
    }

    prom[7] = crc_save;
    return (n_rem >> 12) & 0xF;
}

int bus_diag_ms5611_check(void)
{
    int good = 0;

    for (size_t i = 0; i < ARRAY_SIZE(baros); i++) {
        if (!spi_is_ready_dt(&baros[i])) {
            LOG_ERR("baro%u SPI not ready", (unsigned)(i + 1));
            continue;
        }

        if (ms5611_cmd(&baros[i], MS5611_CMD_RESET) < 0) {
            LOG_ERR("baro%u reset failed", (unsigned)(i + 1));
            continue;
        }
        k_msleep(3); /* datasheet: 2.8 ms reload after reset */

        uint16_t prom[8];
        int ret = 0;
        for (uint8_t w = 0; w < 8 && ret == 0; w++) {
            ret = ms5611_prom_word(&baros[i], w, &prom[w]);
        }
        if (ret < 0) {
            LOG_ERR("baro%u PROM read failed: %d", (unsigned)(i + 1), ret);
            continue;
        }

        /* An unwired bus reads all-zeros or all-ones — reject before CRC
         * (all-zero PROM has a technically valid CRC of 0).
         */
        bool all0 = true, all1 = true;
        for (int w = 0; w < 8; w++) {
            all0 = all0 && (prom[w] == 0x0000);
            all1 = all1 && (prom[w] == 0xFFFF);
        }
        if (all0 || all1) {
            LOG_WRN("baro%u: bus floating (PROM all %s)",
                    (unsigned)(i + 1), all0 ? "0x0000" : "0xFFFF");
            continue;
        }

        uint8_t crc = ms5611_crc4(prom);
        if (crc == (prom[7] & 0xF)) {
            LOG_INF("baro%u: PROM CRC OK (C1=%u C5=%u)",
                    (unsigned)(i + 1), prom[1], prom[5]);
            good++;
        } else {
            LOG_ERR("baro%u: PROM CRC mismatch (calc %u, stored %u)",
                    (unsigned)(i + 1), crc, prom[7] & 0xF);
        }
    }

    LOG_INF("MS5611 check: %d/6 PROM CRCs valid", good);
    return good;
}
