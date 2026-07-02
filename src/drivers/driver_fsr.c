/*
 * FSR (ESS102 force sensor) analog front-end driver.
 *
 * Three TIA outputs (FF1-FF3, OPA2333 stages with R_fb = 100 kΩ) plus
 * the buffered 0.30 V reference rail (VREF) are sampled with the SAADC.
 * Channels come from the board devicetree (zephyr,user io-channels):
 *
 *   idx 0 = AIN0 (P0.02) FF1
 *   idx 1 = AIN1 (P0.03) FF2
 *   idx 2 = AIN2 (P0.04) FF3
 *   idx 3 = AIN3 (P0.05) VREF
 *
 * Sensor resistance for this board's topology (sensor from virtual-
 * ground node to GND, node held at VREF):
 *   R_fsr = R_fb × VREF / (V_out − VREF)
 */

#include "driver_fsr.h"

#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(fsr, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------- */
/*  SAADC channels from devicetree                                            */
/* -------------------------------------------------------------------------- */

#define NUM_ADC_CHANNELS  (FSR_NUM_CHANNELS + 1)

static const struct adc_dt_spec adc_chan[NUM_ADC_CHANNELS] = {
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0),
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1),
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 2),
    ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 3),
};

static int16_t sample_buf;
static struct adc_sequence sequence = {
    .buffer = &sample_buf,
    .buffer_size = sizeof(sample_buf),
};

static int read_channel(uint8_t idx, int32_t *mv)
{
    int ret = adc_sequence_init_dt(&adc_chan[idx], &sequence);
    if (ret < 0) {
        return ret;
    }

    ret = adc_read_dt(&adc_chan[idx], &sequence);
    if (ret < 0) {
        return ret;
    }

    int32_t val = sample_buf;
    if (val < 0) {
        val = 0;  /* single-ended: clamp negative noise around 0 V */
    }

    ret = adc_raw_to_millivolts_dt(&adc_chan[idx], &val);
    if (ret < 0) {
        return ret;
    }

    *mv = val;
    return 0;
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

int drv_fsr_init(void)
{
    for (uint8_t i = 0; i < NUM_ADC_CHANNELS; i++) {
        if (!adc_is_ready_dt(&adc_chan[i])) {
            LOG_ERR("ADC channel %u not ready", i);
            return -ENODEV;
        }

        int ret = adc_channel_setup_dt(&adc_chan[i]);
        if (ret < 0) {
            LOG_ERR("ADC channel %u setup failed: %d", i, ret);
            return ret;
        }
    }

    LOG_INF("SAADC ready: FF1-FF3 (AIN0-2) + VREF (AIN3)");
    return 0;
}

int drv_fsr_read(struct fsr_data *out)
{
    int ret;

    for (uint8_t i = 0; i < FSR_NUM_CHANNELS; i++) {
        ret = read_channel(i, &out->ff_mv[i]);
        if (ret < 0) {
            return ret;
        }
    }

    return read_channel(FSR_NUM_CHANNELS, &out->vref_mv);
}
