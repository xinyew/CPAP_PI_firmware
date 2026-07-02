/*
 * FSR (force-sensitive resistor) analog front-end driver.
 *
 * Three FSR signals (FF1-FF3, buffered by OPA2333 op-amps) plus the
 * op-amp reference rail (VREF) are sampled with the nRF52840 SAADC:
 *
 *   FF1  = AIN0 (P0.02)
 *   FF2  = AIN1 (P0.03)
 *   FF3  = AIN2 (P0.04)
 *   VREF = AIN3 (P0.05)
 *
 * SAADC config: 12-bit, gain 1/6, 0.6 V internal reference
 * → full scale 3.6 V, mv = raw × 3600 / 4096.
 */

#include "driver_fsr.h"

#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_saadc.h>

LOG_MODULE_REGISTER(fsr, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------- */
/*  SAADC setup                                                               */
/* -------------------------------------------------------------------------- */

#define FSR_ADC  DEVICE_DT_GET(DT_NODELABEL(adc))

#define ADC_RESOLUTION      12
#define ADC_FULL_SCALE_MV   3600  /* gain 1/6, ref 0.6 V */

static const struct device *adc_dev = FSR_ADC;

/* Channel IDs 0-3 map to AIN0-AIN3 */
#define NUM_ADC_CHANNELS  (FSR_NUM_CHANNELS + 1)

static int16_t sample_buf;

static int read_channel(uint8_t ch, int32_t *mv)
{
    const struct adc_sequence sequence = {
        .channels    = BIT(ch),
        .buffer      = &sample_buf,
        .buffer_size = sizeof(sample_buf),
        .resolution  = ADC_RESOLUTION,
    };

    int ret = adc_read(adc_dev, &sequence);
    if (ret < 0) {
        return ret;
    }

    int32_t raw = sample_buf;
    if (raw < 0) {
        raw = 0;  /* single-ended: clamp negative noise around 0 V */
    }

    *mv = (raw * ADC_FULL_SCALE_MV) >> ADC_RESOLUTION;
    return 0;
}

/* -------------------------------------------------------------------------- */
/*  Public API                                                                */
/* -------------------------------------------------------------------------- */

int drv_fsr_init(void)
{
    if (!device_is_ready(adc_dev)) {
        LOG_ERR("SAADC not ready");
        return -ENODEV;
    }

    for (uint8_t ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
        const struct adc_channel_cfg cfg = {
            .gain             = ADC_GAIN_1_6,
            .reference        = ADC_REF_INTERNAL,
            .acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10),
            .channel_id       = ch,
            .input_positive   = NRF_SAADC_INPUT_AIN0 + ch,
        };

        int ret = adc_channel_setup(adc_dev, &cfg);
        if (ret < 0) {
            LOG_ERR("ADC channel %u setup failed: %d", ch, ret);
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
