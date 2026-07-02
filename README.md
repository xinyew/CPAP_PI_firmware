# CPAP PI Sensor Body

PPG + force sensing embedded application for a custom nRF52840 board
(Raytac MDBT50Q-P1MV2 module).

Uses **nRF Connect SDK v3.3.0 / Zephyr v4.3.99** architecture.
Debug output via Segger RTT.

## Subsystems

| # | Subsystem | IC | Interface | Purpose |
|---|-----------|-----|-----------|---------|
| 1 | I2C mux | TCA9548A | I2C | Fan out one bus to 3 PPG sensors + temp/humidity |
| 2 | FSR front-end | OPA2333 ×2 | ADC (SAADC) | Buffer 3 force-sensor signals (FF1–FF3) |
| 3 | SPI expansion | — | SPI + 6× CS | Off-board devices via 30-pin FPC |
| 4 | BLE | nRF52840 radio | — | Remote control of sampling interval |
| 5 | Power | XC9140 boost, MIC5504 3.3/1.8 LDOs | — | LiPo battery supply rails |

## Hardware Pin Map

| Signal              | Pin   | Direction | Notes                                      |
|---------------------|-------|-----------|--------------------------------------------|
| FF1 (FSR 1)         | P0.02 | in (AIN0) | Buffered by OPA2333                         |
| FF2 (FSR 2)         | P0.03 | in (AIN1) | Buffered by OPA2333                         |
| FF3 (FSR 3)         | P0.04 | in (AIN2) | Buffered by OPA2333                         |
| VREF                | P0.05 | in (AIN3) | Op-amp reference rail                       |
| I2C SDA             | P0.08 | bidir     | To TCA9548A mux                             |
| I2C SCL             | P0.12 | out       | To TCA9548A mux                             |
| PPG_INT             | P0.11 | in        | PPG sensor interrupt (shared, via FPC)      |
| SPI SCK             | P0.13 | out       | Off-board via 30-pin FPC                    |
| SPI MOSI            | P0.14 | out       | Off-board via 30-pin FPC                    |
| SPI MISO            | P0.21 | in        | Off-board via 30-pin FPC                    |
| CS1–CS6             | P0.07/17/19/20/22/24 | out | Chip-selects for off-board SPI devices |
| TCA_RST             | P1.00 | out       | TCA9548A reset, active-low                  |
| LED1 / LED2         | P1.08 / P1.09 | out | Active-low status LEDs                    |
| nRESET              | P0.18 | —         | Module reset (gpio-as-nreset)               |

## Custom board files
Please refer to ./Boards/kamoamoa/cpap_pi_body/* for available pins, peripherals, and configs

## I2C Bus

- **Single upstream bus** — instance `i2c0` (TWIM0)
- **TCA9548A** mux at 7-bit addr `0x70` (A0=A1=A2=GND), reset on P1.00
- Downstream channels (off-board sensors via 30-pin FPC):
  - ch0–ch2: **PPG sensors 1–3**
  - ch3: **Temp/Humidity sensor**

## SPI Bus

- Instance `spi1` (SPIM1) with six chip-select GPIOs (CS1–CS6)
- All devices off-board via the 30-pin FPC (J1)

## ADC (SAADC)

- 12-bit, gain 1/6, internal 0.6 V reference → 3.6 V full scale
- AIN0–AIN2 = FF1–FF3 force sensors, AIN3 = VREF

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
