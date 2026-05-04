#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <stdio.h>
#include <string.h>

#include "sensors/ess102/ess102_reader.h"
#include "comm/comm_manager.h"

LOG_MODULE_REGISTER(ess102_reader, LOG_LEVEL_INF);

/* 
 * We fetch the ADC specification from the Devicetree overlay.
 * io-channels = <&adc 0>; maps to zephyr,user node
 */
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));

static int16_t sample_buffer[1];

static struct adc_sequence sequence = {
    .buffer = sample_buffer,
    .buffer_size = sizeof(sample_buffer),
    // Optional: .calibrate = true if needed on first run
};

static int ess102_init(void)
{
    LOG_INF("Initializing ESS102 Force Sensor (Differential ADC)...");

    if (!adc_is_ready_dt(&adc_channel)) {
        LOG_ERR("ADC controller device %s not ready", adc_channel.dev->name);
        return -ENODEV;
    }

    int err = adc_channel_setup_dt(&adc_channel);
    if (err < 0) {
        LOG_ERR("Could not setup ADC channel (%d)", err);
        return err;
    }

    // Read the channel info to populate sequence
    err = adc_sequence_init_dt(&adc_channel, &sequence);
    if (err < 0) {
        LOG_ERR("Could not init ADC sequence (%d)", err);
        return err;
    }

    LOG_INF("ESS102 ADC channel perfectly setup! Resolution: %d", sequence.resolution);
    return 0;
}

static int ess102_read(void)
{
    int err;
    int32_t val_mv;

    err = adc_read_dt(&adc_channel, &sequence);
    if (err < 0) {
        LOG_ERR("Could not read ADC (%d)", err);
        return err;
    }

    // The raw sample is in sample_buffer[0]
    val_mv = sample_buffer[0];

    current_sensor_data.force_raw = sample_buffer[0];

    // Convert to millivolts
    err = adc_raw_to_millivolts_dt(&adc_channel, &val_mv);
    
    current_sensor_data.force_mv = val_mv;

    return 0;
}

static uint32_t ess102_get_interval(void)
{
    // Poll the ADC exactly at 60Hz (16ms)
    return 16;
}

sensor_interface_t ess102_sensor = {
    .name = "ESS102 (Force)",
    .init = ess102_init,
    .read = ess102_read,
    .get_interval_ms = ess102_get_interval,
};
