#ifndef MUX_INIT_H
#define MUX_INIT_H

/**
 * @brief Initialize the TCA9548A multiplexer hardware pins.
 * 
 * This ensures the A0, A1, A2 pins are driven low BEFORE I2C init
 * so the MUX is accessible at address 0x70.
 * 
 * @return 0 on success, negative error code on failure.
 */
int mux_init_hardware(void);

#endif /* MUX_INIT_H */
