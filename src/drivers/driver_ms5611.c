/*
 * MS5611-01BA03 barometric pressure sensor driver (SPI, six instances).
 *
 * Protocol shared with the MS5607, but with the MS5611 compensation
 * exponents (datasheet "PRESSURE AND TEMPERATURE CALCULATION"):
 *   dT   = D2 - C5*2^8
 *   TEMP = 2000 + dT*C6 / 2^23                     [0.01 degC]
 *   OFF  = C2*2^16 + C4*dT / 2^7
 *   SENS = C1*2^15 + C3*dT / 2^8
 *   P    = (D1*SENS/2^21 - OFF) / 2^15             [0.01 mbar = 1 Pa]
 * plus second-order compensation below 20 degC.
 *
 * All six sensors share the bus; conversions are started on all of
 * them back-to-back and read after one conversion delay, so the six
 * convert concurrently (this is what makes 25-50 Hz x6 cheap).
 */

#include "driver_ms5611.h"

#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ms5611, LOG_LEVEL_INF);

#define MS5611_SPI_OP  (SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB)

#define CMD_RESET       0x1E
#define CMD_CONV_D1     0x46  /* pressure, OSR 2048 */
#define CMD_CONV_D2     0x56  /* temperature, OSR 2048 */
#define CMD_ADC_READ    0x00
#define CMD_PROM_READ   0xA0  /* + 2*word */

#define CONV_TIME_MS    5     /* OSR 2048 max 4.6 ms */

struct ms5611 {
    struct spi_dt_spec spi;
    uint16_t c[8];        /* calibration PROM */
    int32_t dT;           /* from last D2, used to compensate D1 */
    int32_t temp_c100;
    int32_t press_pa;
    bool ok;
};

static struct ms5611 sensors[MS5611_COUNT] = {
    { .spi = SPI_DT_SPEC_GET(DT_NODELABEL(baro1), MS5611_SPI_OP) },
    { .spi = SPI_DT_SPEC_GET(DT_NODELABEL(baro2), MS5611_SPI_OP) },
    { .spi = SPI_DT_SPEC_GET(DT_NODELABEL(baro3), MS5611_SPI_OP) },
    { .spi = SPI_DT_SPEC_GET(DT_NODELABEL(baro4), MS5611_SPI_OP) },
    { .spi = SPI_DT_SPEC_GET(DT_NODELABEL(baro5), MS5611_SPI_OP) },
    { .spi = SPI_DT_SPEC_GET(DT_NODELABEL(baro6), MS5611_SPI_OP) },
};

static bool conv_is_temp;

/* -------------------------------------------------------------------------- */
/*  SPI helpers                                                               */
/* -------------------------------------------------------------------------- */

static int ms5611_cmd(struct ms5611 *s, uint8_t cmd)
{
    const struct spi_buf tx_buf = { .buf = &cmd, .len = 1 };
    const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };

    return spi_write_dt(&s->spi, &tx);
}

static int ms5611_read_bytes(struct ms5611 *s, uint8_t cmd, uint8_t *data,
                             size_t len)
{
    uint8_t tx_data[4] = { cmd };
    uint8_t rx_data[4];
    const struct spi_buf tx_buf = { .buf = tx_data, .len = len + 1 };
    const struct spi_buf rx_buf = { .buf = rx_data, .len = len + 1 };
    const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
    const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

    int ret = spi_transceive_dt(&s->spi, &tx, &rx);
    if (ret == 0) {
        memcpy(data, &rx_data[1], len);
    }
    return ret;
}

static int ms5611_read_adc(struct ms5611 *s, uint32_t *val)
{
    uint8_t b[3];
    int ret = ms5611_read_bytes(s, CMD_ADC_READ, b, 3);

    *val = ((uint32_t)b[0] << 16) | ((uint32_t)b[1] << 8) | b[2];
    return ret;
}

/* CRC-4 over the calibration PROM (TE AN520) */
static uint8_t ms5611_crc4(uint16_t prom[8])
{
    uint16_t n_rem = 0;
    uint16_t crc_save = prom[7];

    prom[7] &= 0xFF00;
    for (int cnt = 0; cnt < 16; cnt++) {
        if (cnt & 1) {
            n_rem ^= prom[cnt >> 1] & 0x00FF;
        } else {
            n_rem ^= prom[cnt >> 1] >> 8;
        }
        for (int bit = 8; bit > 0; bit--) {
            n_rem = (n_rem & 0x8000) ? (n_rem << 1) ^ 0x3000 : n_rem << 1;
        }
    }
    prom[7] = crc_save;
    return (n_rem >> 12) & 0xF;
}

/* -------------------------------------------------------------------------- */
/*  Compensation (MS5611 exponents, incl. second order)                       */
/* -------------------------------------------------------------------------- */

static void ms5611_update_temp(struct ms5611 *s, uint32_t d2)
{
    s->dT = (int32_t)d2 - ((int32_t)s->c[5] << 8);
    s->temp_c100 = 2000 + (int32_t)(((int64_t)s->dT * s->c[6]) >> 23);
}

static void ms5611_update_press(struct ms5611 *s, uint32_t d1)
{
    int64_t off  = ((int64_t)s->c[2] << 16) + (((int64_t)s->c[4] * s->dT) >> 7);
    int64_t sens = ((int64_t)s->c[1] << 15) + (((int64_t)s->c[3] * s->dT) >> 8);
    int32_t temp = s->temp_c100;

    if (temp < 2000) {
        int64_t t2    = ((int64_t)s->dT * s->dT) >> 31;
        int64_t tsq   = (int64_t)(temp - 2000) * (temp - 2000);
        int64_t off2  = (5 * tsq) >> 1;
        int64_t sens2 = (5 * tsq) >> 2;

        if (temp < -1500) {
            int64_t tsq2 = (int64_t)(temp + 1500) * (temp + 1500);

            off2  += 7 * tsq2;
            sens2 += (11 * tsq2) >> 1;
        }
        s->temp_c100 = temp - (int32_t)t2;
        off  -= off2;
        sens -= sens2;
    }

    s->press_pa = (int32_t)(((((int64_t)d1 * sens) >> 21) - off) >> 15);
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

int drv_ms5611_init(void)
{
    int good = 0;

    for (int i = 0; i < MS5611_COUNT; i++) {
        struct ms5611 *s = &sensors[i];

        s->ok = false;
        if (!spi_is_ready_dt(&s->spi)) {
            continue;
        }
        if (ms5611_cmd(s, CMD_RESET) < 0) {
            continue;
        }
        k_msleep(3);

        bool all0 = true, all1 = true;
        int ret = 0;

        for (uint8_t w = 0; w < 8 && ret == 0; w++) {
            uint8_t b[2];

            ret = ms5611_read_bytes(s, CMD_PROM_READ + 2 * w, b, 2);
            s->c[w] = ((uint16_t)b[0] << 8) | b[1];
            all0 = all0 && (s->c[w] == 0x0000);
            all1 = all1 && (s->c[w] == 0xFFFF);
        }
        if (ret < 0 || all0 || all1 ||
            ms5611_crc4(s->c) != (s->c[7] & 0xF)) {
            LOG_WRN("baro%d absent or bad PROM", i + 1);
            continue;
        }

        s->ok = true;
        good++;
    }

    /* Seed dT with one blocking temperature conversion */
    if (good > 0) {
        drv_ms5611_start_conv(true);
        k_msleep(CONV_TIME_MS + 1);
        drv_ms5611_finish_conv();
    }

    LOG_INF("MS5611: %d/6 online", good);
    return good;
}

uint8_t drv_ms5611_ok_mask(void)
{
    uint8_t mask = 0;

    for (int i = 0; i < MS5611_COUNT; i++) {
        if (sensors[i].ok) {
            mask |= BIT(i);
        }
    }
    return mask;
}

int drv_ms5611_start_conv(bool temperature)
{
    int ret = 0;

    conv_is_temp = temperature;
    for (int i = 0; i < MS5611_COUNT; i++) {
        if (!sensors[i].ok) {
            continue;
        }
        int r = ms5611_cmd(&sensors[i],
                           temperature ? CMD_CONV_D2 : CMD_CONV_D1);
        if (r < 0 && ret == 0) {
            ret = r;
        }
    }
    return ret;
}

int drv_ms5611_finish_conv(void)
{
    int ret = 0;

    for (int i = 0; i < MS5611_COUNT; i++) {
        if (!sensors[i].ok) {
            continue;
        }

        uint32_t adc;
        int r = ms5611_read_adc(&sensors[i], &adc);

        if (r < 0) {
            if (ret == 0) {
                ret = r;
            }
            continue;
        }
        if (adc == 0) {
            continue;  /* conversion not ready — keep previous value */
        }
        if (conv_is_temp) {
            ms5611_update_temp(&sensors[i], adc);
        } else {
            ms5611_update_press(&sensors[i], adc);
        }
    }
    return ret;
}

void drv_ms5611_get(int idx, int32_t *press_pa, int32_t *temp_c100)
{
    if (idx < 0 || idx >= MS5611_COUNT || !sensors[idx].ok) {
        *press_pa = 0;
        *temp_c100 = 0;
        return;
    }
    *press_pa = sensors[idx].press_pa;
    *temp_c100 = sensors[idx].temp_c100;
}
