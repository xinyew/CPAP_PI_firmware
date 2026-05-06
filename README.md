# CPAP Pressure Injury Sensing Firmware

This repository contains the firmware for a multi-sensor CPAP pressure injury monitoring system built on the **nRF52840 DK** using the **Zephyr RTOS** (nRF Connect SDK).

## System Overview

The system monitors three primary physiological and environmental parameters at high frequency (60Hz) to detect early signs of pressure injuries caused by CPAP masks:
1. **Pulse Oximetry (PPG)**: Heart rate and SpO2 trends via MAX30101.
2. **Contact Pressure**: Real-time force measurement via ESS102 (Differential ADC).
3. **Environment**: Micro-climate temperature and humidity via SHT40.

## Key Features

- **Unified 60Hz Stream**: High-performance JSON reporting cadence optimized for real-time visualization.
- **Dynamic Comm Switching**: Toggle between **Wired (USB Serial)** and **Wireless (BLE)** at the press of a button.
- **Dual-Interface Advertising**: Implements the Nordic UART Service (NUS) for seamless browser integration via Web Bluetooth.
- **Hardware Abstraction**: Handles I2C multiplexing (TCA9548A) and differential analog-to-digital conversion.

## Hardware Configuration

### Pin Mapping (nRF52840 DK)
- **I2C SDA/SCL**: P0.27 / P0.26
- **MUX Reset**: P0.02
- **MAX30101 IRQ**: P1.13
- **ESS102 Force Sensor (TIA Architecture)**:
    - **P0.03 (AIN1)**: Measured Buffered Vref (used for exact resistance calculation).
    - **P0.05 (AIN3)**: TIA Output Voltage ($V_{out}$).
    - **Calculation**: $R_{fsr} = R_{fb} \cdot (V_{ref} / V_{out})$.
- **User Interface**:
    - **Button 1**: Cycle Communication Mode (Serial -> BLE).
    - **LED 1**: Serial Mode Indicator.
    - **LED 2**: BLE Mode Indicator.

## Getting Started

### Prerequisites
- nRF Connect SDK (v2.x or v3.x)
- VS Code with nRF Connect Extension Pack

### Building & Flashing
1. Open the project in VS Code / nRF Connect.
2. Create a build configuration for `nrf52840dk/nrf52840`.
3. Build the project:
   ```bash
   west build -b nrf52840dk/nrf52840
   ```
4. Flash to your board:
   ```bash
   west flash
   ```

## Serial Protocol
The board emits a compact JSON object at 60Hz:
```json
{"r":123,"i":456,"g":789,"f":450,"v":3300,"res":12000.5,"t":24.5,"h":45.2}
```
- `r/i/g`: Red, IR, and Green PPG values.
- `f`: TIA Output Voltage (mV).
- `v`: Measured Vref (mV).
- `res`: Calculated FSR Resistance ($\Omega$).
- `t/h`: Temperature (°C) and Humidity (%).
