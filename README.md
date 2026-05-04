# CPAP Pressure Injury Sensing Application

This is an nRF Connect SDK project for the nRF52840 DK board. 

## Project Architecture
This project utilizes a hybrid C/C++ architecture. Core RTOS components (like `sensor_manager` and `comm_manager`) are written in C, while specific sensor drivers leverage mature open-source Arduino C++ libraries.

We use a custom **Arduino Compatibility Layer** (`src/arduino_compat/`) that implements the Arduino `TwoWire` (`Wire.h`) and `Arduino.h` APIs. This allows standard Arduino C++ libraries to run natively on Zephyr by seamlessly routing their I2C calls into Zephyr's high-speed `i2c_write` and `i2c_read` functions.

## Hardware Wiring & Connections

Below is the definitive pin mapping for the nRF52840 DK. All I2C sensors are routed through the TCA9548A multiplexer.

| Component | Pin Function | nRF52840 DK Pin | Notes |
| :--- | :--- | :--- | :--- |
| **I2C Bus (MUX)** | SDA | `P0.27` | Main I2C Data line to TCA9548A |
| | SCL | `P0.26` | Main I2C Clock line to TCA9548A |
| | Reset | `P0.02` | MUX Reset (Active Low) |
| **MUX Addressing** | A0 | `P1.11` | Forced LOW via Zephyr GPIO |
| | A1 | `P1.15` | Forced LOW via Zephyr GPIO |
| | A2 | `P1.14` | Forced LOW via Zephyr GPIO |
| **MAX30101 (PPG)**| INT | `P1.13` | Interrupt pin (Active Low) |
| **ESS102 (Force)**| Positive Input | `P0.03` (AIN1) | Sensor Output signal |
| | Negative Input | `P0.05` (AIN3) | 0.3V External Reference |

> [!IMPORTANT]
> **Force Sensor Differential ADC**: The ESS102 force sensor is connected using a differential ADC measurement because the output signal may fall below the reference voltage. It is critical that the 0.3V reference goes to `P0.05` and the sensor output goes to `P0.03`. **Do not apply voltages below GND directly to any pin, as this will destroy the microcontroller.**

## Sensor Implementations

- **Phase 1: MAX30101 Pulse Oximeter**: 
  - Behind I2C MUX Channel 0 (Address: 0x57)
  - Driven by the **EmotiBit MAX30101 C++ Library**.
  - Configured for Multi-LED (Red, IR, Green) at 50Hz.
  - The Zephyr built-in driver is disabled (`compatible = "emotibit,max30101"`) to prevent hardware conflicts.

- **Phase 2: SHT40 Temperature & Humidity**:
  - Behind I2C MUX Channel 1 (Address: 0x44)
  - Driven by the **MR01Right SHT40 C++ Library**.
  - Utilizes our upgraded `Wire1` polyfill, allowing Zephyr to automatically switch MUX channels in the background during I2C reads.

- **Phase 3: ESS102 Force Sensor**:
  - Connected directly to the nRF52840's internal SAADC.
  - Configured for 12-bit Differential measurement using the internal 0.6V reference and a 1/4 gain setting.
