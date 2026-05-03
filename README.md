# CPAP Pressure Injury Sensing Application

This is an nRF Connect SDK project for the nRF52840 DK board. 

## Project Architecture
This project utilizes a hybrid C/C++ architecture. Core RTOS components (like `sensor_manager` and `comm_manager`) are written in C, while specific sensor drivers leverage mature open-source Arduino C++ libraries.

We use a custom **Arduino Compatibility Layer** (`src/arduino_compat/`) that implements the Arduino `TwoWire` (`Wire.h`) and `Arduino.h` APIs. This allows standard Arduino C++ libraries to run natively on Zephyr by seamlessly routing their I2C calls into Zephyr's high-speed `i2c_write` and `i2c_read` functions.

## Hardware Configured (Phase 1: Complete)
- **I2C MUX (TCA9548A)**:
  - Main I2C Bus: SDA (P0.27), SCL (P0.26)
  - Address Pins: A0 (P1.11), A1 (P1.15), A2 (P1.14) forced LOW.
  - Address: 0x70
  - The MUX is initialized using Zephyr's `SYS_INIT(..., POST_KERNEL, 40)` to guarantee its GPIO address pins are driven LOW before the I2C bus initializes.

- **MAX30101 Pulse Oximeter**: 
  - Behind I2C MUX Channel 0
  - Address: 0x57
  - Driven by the **EmotiBit MAX30101 C++ Library** (via git submodule).
  - Configured for Multi-LED (Red, IR, Green) at 50Hz.
  - The Zephyr built-in driver is disabled (`compatible = "emotibit,max30101"`) to avoid hardware conflicts, and the EmotiBit algorithm pulls raw FIFO data directly.
