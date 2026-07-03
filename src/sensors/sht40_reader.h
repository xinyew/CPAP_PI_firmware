#ifndef SHT40_READER_H
#define SHT40_READER_H

#include <zephyr/kernel.h>

/**
 * @brief Check the SHT40 (in-tree driver, mux ch3, high repeatability).
 *
 * @return 0 if present, negative errno otherwise.
 */
int sht40_reader_init(void);

/**
 * @brief Blocking measurement (~8.3 ms at high repeatability).
 *
 * @param temp_c100  Temperature in 0.01 degC.
 * @param rh_x100    Relative humidity in 0.01 %RH.
 * @return 0 on success, negative errno on failure.
 */
int sht40_reader_read(int32_t *temp_c100, int32_t *rh_x100);

#endif /* SHT40_READER_H */
