# CPAP Pressure Injury Sensing Application

This is an nRF Connect SDK project for the nRF52840 DK board. This branch focuses strictly on the initial integration of the I2C MUX and the MAX30101 sensor.

## Hardware Configured (Phase 1)
- **I2C MUX (TCA9548A)**:
  - Main I2C Bus: SDA (P0.27), SCL (P0.26)
  - Address Pins: A0 (P1.11), A1 (P1.15), A2 (P1.14) forced LOW.
  - Address: 0x70
- **MAX30101**: 
  - Behind I2C MUX Channel 0
  - Address: 0x57
  - Interrupt Pin: P1.13
