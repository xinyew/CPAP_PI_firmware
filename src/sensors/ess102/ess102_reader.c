#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <stdio.h>
#include <string.h>

#include "sensors/ess102/ess102_reader.h"
#include "comm/comm_manager.h"

LOG_MODULE_REGISTER(ess102_reader, LOG_LEVEL_WRN);

/* 
 * Feedback Resistor (Rfb) in Ohms. 
 * Update this value to match your physical circuit.
 */
#define RFB_OHM 10000.0f

/* 
 * We fetch the ADC specifications from the Devicetree overlay.
 * io-channels = <&adc 0>, <&adc 1>; maps to zephyr,user node
 */
static const struct adc_dt_spec adc_vref = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static const struct adc_dt_spec adc_vout = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 1);

static int16_t vref_buffer[1];
static int16_t vout_buffer[1];

static struct adc_sequence vref_seq = {
    .buffer = vref_buffer,
    .buffer_size = sizeof(vref_buffer),
};

static struct adc_sequence vout_seq = {
    .buffer = vout_buffer,
    .buffer_size = sizeof(vout_buffer),
};

static int ess102_init(void)
{
    LOG_INF("Initializing ESS102 Force Sensor (TIA + Measured Vref)...");

    if (!adc_is_ready_dt(&adc_vref) || !adc_is_ready_dt(&adc_vout)) {
        LOG_ERR("ADC controller not ready");
        return -ENODEV;
    }

    int err = adc_channel_setup_dt(&adc_vref);
    if (err < 0) return err;

    err = adc_channel_setup_dt(&adc_vout);
    if (err < 0) return err;

    adc_sequence_init_dt(&adc_vref, &vref_seq);
    adc_sequence_init_dt(&adc_vout, &vout_seq);

    LOG_INF("ESS102 Dual-Channel ADC setup complete.");
    return 0;
}

static int ess102_read(void)
{
    int err;
    int32_t vref_mv, vout_mv;

    // 1. Read Vref (P0.03)
    err = adc_read_dt(&adc_vref, &vref_seq);
    if (err < 0) return err;
    vref_mv = vref_buffer[0];
    adc_raw_to_millivolts_dt(&adc_vref, &vref_mv);

    // 2. Read Vout (P0.05)
    err = adc_read_dt(&adc_vout, &vout_seq);
    if (err < 0) return err;
    vout_mv = vout_buffer[0];
    adc_raw_to_millivolts_dt(&adc_vout, &vout_mv);

    // 3. Store raw/mv data
    current_sensor_data.force_vref_mv = vref_mv;
    current_sensor_data.force_mv = vout_mv;
    current_sensor_data.force_raw = vout_buffer[0];

    // 4. Calculate Resistance (Rfsr)
    // Circuit: Inverting TIA with +input at Vref (Baseline), FSR between -input and GND.
    // Formula: Vout = Vref - (Vref/Rfsr) * Rfb  => Rfsr = (Vref * Rfb) / (Vref - Vout)
    
    int32_t diff = vref_mv - vout_mv; // Current is proportional to how much Vout drops below Vref
    if (diff > 5) { // Threshold for minimum current flow
        current_sensor_data.force_res_ohm = RFB_OHM * ((float)vref_mv / (float)diff);
    } else {
        current_sensor_data.force_res_ohm = 1000000.0f; // High value for open circuit/no force
    }

    return 0;
}

static uint32_t ess102_get_interval(void)
{
    return 16; // 60Hz
}

sensor_interface_t ess102_sensor = {
    .name = "ESS102 (Force)",
    .init = ess102_init,
    .read = ess102_read,
    .get_interval_ms = ess102_get_interval,
};
