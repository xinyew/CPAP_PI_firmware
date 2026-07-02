#ifndef DRIVER_FSR_H
#define DRIVER_FSR_H

#include <zephyr/kernel.h>

/* Number of force-sensor channels (FF1-FF3) */
#define FSR_NUM_CHANNELS    3

/* One sample set: three FSR channels + the op-amp reference rail */
struct fsr_data {
    int32_t ff_mv[FSR_NUM_CHANNELS];  /* FF1-FF3 in millivolts */
    int32_t vref_mv;                  /* VREF (AIN3) in millivolts */
};

/**
 * @brief Initialize the SAADC channels for the FSR front-end.
 *
 * FF1 = AIN0 (P0.02), FF2 = AIN1 (P0.03), FF3 = AIN2 (P0.04),
 * VREF = AIN3 (P0.05). The FSR signals are buffered by OPA2333 op-amps.
 *
 * @return 0 on success, negative errno on failure.
 */
int drv_fsr_init(void);

/**
 * @brief Read all FSR channels and the reference rail.
 *
 * @param out  Destination for the converted sample set.
 * @return 0 on success, negative errno on failure.
 */
int drv_fsr_read(struct fsr_data *out);

#endif /* DRIVER_FSR_H */
