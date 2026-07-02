# CPAP PI Sensor Control

PPG + force + pressure sensing embedded application for a custom nRF52840
control board (Raytac MDBT50Q-P1MV2 module), driving the CPAP PI sensor
mask flex PCB over a 30-pin FPC.

Uses **nRF Connect SDK v3.3.0 / Zephyr v4.3.99** architecture.
Debug output via Segger RTT.

## Subsystems

| # | Subsystem | IC | Interface | Purpose |
|---|-----------|-----|-----------|---------|
| 1 | I2C mux | TCA9548A | I2C | Fan out one bus to 3× MAX30101 + SHT40 |
| 2 | PPG | MAX30101 ×3 (mask flex) | I2C via mux ch0–2 | Pulse oximetry at 3 mask sites |
| 3 | Temp/Humidity | SHT40 (mask flex) | I2C via mux ch3 | Mask micro-climate |
| 4 | Contact pressure | MS5611 ×6 (mask flex) | SPI, CS1–CS6 | Barometric pressure sensing |
| 5 | Force front-end | ESS102 ×3 + OPA2333 ×2 | ADC (SAADC) | TIA-buffered force channels FF1–FF3 |
| 6 | BLE | nRF52840 radio | — | Remote control of sampling interval |
| 7 | Power | XC9140 boost, MIC5504 3.3/1.8 LDOs | — | LiPo battery supply rails |

## Hardware Pin Map

| Signal              | Pin   | Direction | Notes                                      |
|---------------------|-------|-----------|--------------------------------------------|
| FF1 (ESS102 1)      | P0.02 | in (AIN0) | TIA output, buffered by OPA2333             |
| FF2 (ESS102 2)      | P0.03 | in (AIN1) | TIA output, buffered by OPA2333             |
| FF3 (ESS102 3)      | P0.04 | in (AIN2) | TIA output, buffered by OPA2333             |
| VREF                | P0.05 | in (AIN3) | Op-amp reference rail                       |
| I2C SDA             | P0.08 | bidir     | To TCA9548A mux                             |
| I2C SCL             | P0.12 | out       | To TCA9548A mux                             |
| PPG_INT             | P0.11 | in        | MAX30101 interrupt (shared, via FPC)        |
| SPI SCK             | P0.13 | out       | To mask flex via 30-pin FPC                 |
| SPI MOSI            | P0.14 | out       | To mask flex via 30-pin FPC                 |
| SPI MISO            | P0.21 | in        | From mask flex via 30-pin FPC               |
| CS1–CS6             | P0.07/17/19/20/22/24 | out | Chip-selects for MS5611 baros 1–6     |
| TCA_RST             | P1.00 | out       | TCA9548A reset, active-low                  |
| LED1 / LED2         | P1.08 / P1.09 | out | Active-low status LEDs                    |
| nRESET              | P0.18 | —         | Module reset (gpio-as-nreset)               |

## Custom board files
Please refer to ./Boards/kamoamoa/cpap_pi_control/* for available pins, peripherals, and configs

## I2C Bus

- **Single upstream bus** — instance `i2c0` (TWIM0)
- **TCA9548A** mux at 7-bit addr `0x70` (A0=A1=A2=GND), reset on P1.00
- Downstream channels (sensors on the mask flex PCB via 30-pin FPC):
  - ch0–ch2: **MAX30101** PPG sensors 1–3 (`0x57` each)
  - ch3: **SHT40** temp/humidity (`0x44`)

## SPI Bus

- Instance `spi1` (SPIM1) with six chip-select GPIOs (CS1–CS6)
- Six **MS5611** barometric pressure sensors on the mask flex PCB

## ADC (SAADC)

- 12-bit, gain 1/6, internal 0.6 V reference → 3.6 V full scale
- AIN0–AIN2 = ESS102 force channels FF1–FF3 (TIA outputs), AIN3 = VREF

## Companion hardware

- Control board schematic: `CPAP_PI_Sensor_Body.kicad_sch` (KiCad project name predates the _control rename)
- Mask flex PCB schematic: `CPAP_PI_Sensor_Mask.kicad_sch` — carries all sensors listed above

## src dir structure (beginning)
├── src/
│   ├── main.c
│   ├── drivers/
│   │   ├── driver_tca9548a.c/h           # I2C mux channel select/reset
│   │   ├── driver_fsr.c/h                # SAADC force-sensor sampling
│   │   ├── driver_led.c/h                # Status LEDs
│   │   └── ... other drivers
│   └── interface/
│       └── ble_interface.c/h             # GATT server, sampling control
