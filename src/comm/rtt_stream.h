#ifndef RTT_STREAM_H
#define RTT_STREAM_H

#include <zephyr/kernel.h>

/**
 * @brief Configure RTT up-buffer 1 ("data") for the wired binary stream.
 *
 * Same DATA/STATUS frames as BLE (see comm_protocol.h), always active
 * regardless of BLE state. Non-blocking: bytes are silently skipped
 * when no debug probe is draining the buffer, so the firmware never
 * stalls untethered.
 *
 * Read on the PC with scripts/rtt_bridge.py (RTT -> WebSocket for the
 * web portal). RTT is a byte stream — the bridge re-frames on the
 * 0xC9A5 magic.
 *
 * @return 0 on success, negative on RTT error.
 */
int rtt_stream_init(void);

/** @brief Write one frame to the RTT data channel (cheap memcpy). */
void rtt_stream_write(const uint8_t *data, uint16_t len);

#endif /* RTT_STREAM_H */
