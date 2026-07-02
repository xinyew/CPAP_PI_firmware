# CLAUDE.md — Agent Coding Instructions

> Full project docs in README.md. This file is the compact reference for coding.

## Quick Ref

- **SDK**: nRF Connect SDK v3.3.0 / Zephyr v4.3.99
- **Module**: Raytac MDBT50Q-P1MV2 (nRF52840)
- **Board**: `cpap_pi_body/nrf52840` (custom, in `Boards/kamoamoa/cpap_pi_body/`)
- **Build**: `cd build && ninja` (app image in `build/Moamoa_CPAP_PI_firmware/`)
- **Ninja**: `C:\Users\xwang3239\ncs\toolchains\936afb6332\opt\bin\ninja.exe`
- **Console**: RTT only (`printk()`), no UART. No `%f` — use fixed-point ints. Enable float-printing when necessary

## Commands:
Load nrf connect sdk terminal for env variables, etc, please do this before any command below other than the RTT reading:
$ nrfutil sdk-manager toolchain launch --ncs-version v3.3.0 --terminal
Build project (the `-- -DBOARD_ROOT=.` is required on the first/pristine configure so sysbuild finds the custom board):
$ west build -b cpap_pi_body/nrf52840 -- -DBOARD_ROOT=.
Flash firmware after a successful build:
$ west flash --recover
Load RTT console and connect to the board (makesure to set a timeout of 5 seconds, if there's nothing or returned meaning no output):
$ & "~\Downloads\SimplicityCommander-Windows\SimplicityCommander-Windows\Commander-cli_win32_x64_1v24p1b1980\Simplicity Commander CLI\commander-cli.exe"  rtt connect --device nrf52840_M4

## Pin Map

| Pin   | Function          | Notes                              |
|-------|-------------------|------------------------------------|
| P0.02 | AIN0 (in)         | FF1 — FSR 1 via OPA2333            |
| P0.03 | AIN1 (in)         | FF2 — FSR 2 via OPA2333            |
| P0.04 | AIN2 (in)         | FF3 — FSR 3 via OPA2333            |
| P0.05 | AIN3 (in)         | VREF — op-amp reference rail       |
| P0.07 | CS1 (out)         | SPI chip-select 1 (off-board)      |
| P0.08 | I2C SDA           | To TCA9548A mux                    |
| P0.11 | PPG_INT (in)      | PPG sensor interrupt               |
| P0.12 | I2C SCL           | To TCA9548A mux                    |
| P0.13 | SPI SCK (out)     | Off-board via 30-pin FPC           |
| P0.14 | SPI MOSI (out)    | Off-board via 30-pin FPC           |
| P0.17 | CS2 (out)         | SPI chip-select 2                  |
| P0.18 | nRESET            | gpio-as-nreset                     |
| P0.19 | CS3 (out)         | SPI chip-select 3                  |
| P0.20 | CS4 (out)         | SPI chip-select 4                  |
| P0.21 | SPI MISO (in)     | Off-board via 30-pin FPC           |
| P0.22 | CS5 (out)         | SPI chip-select 5                  |
| P0.24 | CS6 (out)         | SPI chip-select 6                  |
| P1.00 | TCA_RST (out)     | TCA9548A reset, active-low         |
| P1.08 | LED1 (out)        | Active-low                         |
| P1.09 | LED2 (out)        | Active-low                         |

## I2C Addresses (7-bit)

- TCA9548A (mux): `0x70` (A0=A1=A2=GND)
- PPG sensors ×3: behind mux ch0–ch2 (SDA/SCL_PPG1-3, off-board)
- Temp/Humidity: behind mux ch3 (SDA/SCL_TH, off-board)

## SOC components to use
i2c0 (TWIM0, to mux), spi1 (SPIM1, 6 cs-gpios), adc (SAADC AIN0-3), bunch of gpios.
32.768 kHz crystal fitted → K32SRC_XTAL. NOTE: i2c0 must not be used together with spi0, and i2c1 not with spi1 (shared peripheral IDs).
