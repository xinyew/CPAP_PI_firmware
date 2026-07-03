#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <zephyr/kernel.h>

#include "ppg_reader.h"

/* One 10 ms sampling tick — the unit batched into BLE DATA frames. */
struct tick_sample {
    struct ppg_sample ppg[PPG_COUNT];
    int16_t ff_mv[3];
    int16_t vref_mv;
    int32_t baro_pa[6];   /* baro also samples per tick (100 Hz) */
};

/* Latest values from every sensor, updated by the sampling thread. */
struct system_sensor_data {
    /* PPG (MAX30101 x3, 100 Hz) */
    struct ppg_sample ppg[PPG_COUNT];

    /* FSR front-end (100 Hz) */
    int32_t ff_mv[3];
    int32_t vref_mv;
    int32_t rfsr_ohm[3];      /* -1 = open / below reference */

    /* MS5611 x6 (pressure ~25 Hz, temperature 1 Hz) */
    int32_t baro_pa[6];       /* Pa (= 0.01 mbar) */
    int32_t baro_temp_c100[6];
    uint8_t baro_ok_mask;

    /* SHT40 (1 Hz) */
    int32_t sht_temp_c100;
    int32_t sht_rh_x100;
    bool sht_ok;

    /* Measured rates over the last summary window (per second) */
    uint32_t ppg_rate;
    uint32_t fsr_rate;
    uint32_t baro_rate;
};

extern struct system_sensor_data g_sensor_data;

/**
 * @brief Initialize all sensors and start the 100 Hz sampling thread.
 *
 * Prints a one-line status summary to RTT every second (rates + latest
 * values) — the phase 3 "all signals live" gate.
 *
 * @return 0 on success (starts even with sensors absent).
 */
int sensor_manager_start(void);

#endif /* SENSOR_MANAGER_H */
