#ifndef COMM_PROTOCOL_H
#define COMM_PROTOCOL_H

/*
 * Binary stream protocol over BLE NUS (little-endian).
 *
 * DATA frame — every 40 ms (25/s), batching 4 sensor ticks (10 ms each):
 *
 *   off size field
 *     0  u16  magic 0xC9A5
 *     2  u8   type = 0x01
 *     3  u8   seq (wraps)
 *     4  u32  t_ms (uptime of last tick in frame)
 *     8  u8   ppg_valid_mask (bit N = ppg N+1 live)
 *     9  u8   n_samples (= 4)
 *    10  u8   baro_ok_mask (bit N = baro N+1 live)
 *    11  u8   reserved
 *    12  PPG block: sensor 0..2, sample 0..3: red, ir, green as u24
 *         = 3 x 4 x 3 x 3 B = 108 B (absent sensors zero-filled)
 *   120  FSR block: sample 0..3: ff1, ff2, ff3, vref as i16 mV = 32 B
 *   152  BARO block: sample 0..3, baro 1..6: pressure as u24 Pa
 *         = 4 x 6 x 3 B = 72 B (baro samples at 100 Hz too)
 *   224  total (fits one NUS notification at ATT MTU >= 227)
 *
 * STATUS frame — every 1 s:
 *
 *     0  u16  magic 0xC9A5
 *     2  u8   type = 0x02
 *     3  u8   seq
 *     4  u32  t_ms
 *     8  i16  sht_temp (0.01 degC)
 *    10  u16  sht_rh (0.01 %RH)
 *    12  i16  baro_temp[6] (0.01 degC) = 12 B
 *    24  i32  rfsr_ohm[3] (-1 = open/invalid) = 12 B
 *    36  u8   rate_ppg, rate_fsr, rate_baro (Hz achieved)
 *    39  u8   ppg_valid_mask
 *    40  u8   baro_ok_mask
 *    41  total
 *
 * NUS RX commands (single ASCII byte):
 *   'B' — binary streaming (default)
 *   'J' — JSON debug mode: 1 Hz eval-style JSON line instead of frames
 */

#define COMM_MAGIC          0xC9A5
#define COMM_TYPE_DATA      0x01
#define COMM_TYPE_STATUS    0x02

#define COMM_TICKS_PER_FRAME  4
#define COMM_DATA_FRAME_LEN   224
#define COMM_STATUS_FRAME_LEN 41

#define COMM_CMD_BINARY     'B'
#define COMM_CMD_JSON       'J'

#endif /* COMM_PROTOCOL_H */
