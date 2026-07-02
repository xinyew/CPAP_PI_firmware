#ifndef BUS_DIAG_H
#define BUS_DIAG_H

#include <zephyr/kernel.h>

/**
 * @brief Scan all four TCA9548A downstream channels.
 *
 * Probes every 7-bit address on each mux channel bus and prints what
 * ACKs. Expected: 0x57 (MAX30101) on ch0-ch2, 0x44 (SHT40) on ch3.
 *
 * @return Number of expected devices found (0-4).
 */
int bus_diag_scan_mux(void);

/**
 * @brief Reset all six MS5611 barometers and validate their PROM CRCs.
 *
 * Sends the reset command, reads the 8-word calibration PROM per sensor
 * and checks the CRC-4 (datasheet AN520 algorithm).
 *
 * @return Number of sensors with a valid PROM CRC (0-6).
 */
int bus_diag_ms5611_check(void);

#endif /* BUS_DIAG_H */
