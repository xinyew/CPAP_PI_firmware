# CPAP Pressure Injury Sensing Firmware

This repository contains the firmware for a multi-sensor CPAP pressure injury monitoring system built on the **nRF52840 DK** using the **Zephyr RTOS** (nRF Connect SDK).

## System Overview

The system monitors four primary physiological and environmental parameters at high frequency to detect early signs of pressure injuries caused by CPAP masks:
1. **Pulse Oximetry (PPG)**: Heart rate and SpO2 trends via MAX30101.
2. **Contact Pressure**: Real-time force measurement via ESS102 (Differential ADC).
3. **Environment**: Micro-climate temperature and humidity via SHT40.
4. **Barometric Pressure**: High-precision ambient/mask pressure and temperature via MS5611 (GY-63).

## Key Features

- **Unified 60Hz Stream**: High-performance JSON reporting cadence optimized for real-time visualization.
- **Dynamic Comm Switching**: Toggle between **Wired (USB Serial)** and **Wireless (BLE)** at the press of a button.
- **Dual-Interface Advertising**: Implements the Nordic UART Service (NUS) for seamless browser integration via Web Bluetooth.
- **Hardware Abstraction**: Handles I2C multiplexing (TCA9548A) to allow safe co-existence of SHT40 and MS5611 on I2C MUX Channel 1, and differential analog-to-digital conversion (ADC) for the force sensor.

## Hardware Configuration

### Pin Mapping (nRF52840 DK)
- **I2C SDA/SCL**: P0.27 / P0.26
- **MUX Reset**: P0.02
- **MAX30101 IRQ**: P1.13
- **ESS102 Force Sensor (TIA Architecture)**:
    - **P0.03 (AIN1)**: Measured Buffered Vref (used for exact resistance calculation).
    - **P0.05 (AIN3)**: TIA Output Voltage ($V_{out}$).
    - **Calculation**: $R_{fsr} = R_{fb} \cdot (V_{ref} / V_{out})$.
- **SHT40 Temp/Humidity & MS5611 Pressure**:
    - Both reside on TCA9548A Multiplexer Channel 1 (`mux_i2c1`).
    - MS5611 auto-detects CSB pin pulling configuration between address `0x77` (CSB high/floating) and `0x76` (CSB low).
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
2. Create a build configuration for `nrf52840dk_nrf52840`.
3. Build the project:
   ```bash
   west build -b nrf52840dk_nrf52840
   ```
4. Flash to your board:
   ```bash
   west flash
   ```

## Serial Protocol
The board emits a compact JSON object at high frequency:
```json
{"r":123,"i":456,"g":789,"f":450,"v":3300,"res":12000.5,"t":24.5,"h":45.2,"p":1013.25,"pt":24.6}
```
- `r/i/g`: Red, IR, and Green PPG values from MAX30101.
- `f`: TIA Output Voltage (mV).
- `v`: Measured Vref (mV).
- `res`: Calculated FSR Resistance ($\Omega$).
- `t/h`: Ambient Temperature (°C) and Relative Humidity (%) from SHT40.
- `p`: Barometric Pressure (mbar) from MS5611 (polled at 10Hz).
- `pt`: Pressure Sensor Temperature (°C) from MS5611.
