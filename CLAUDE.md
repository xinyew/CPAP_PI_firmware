# CLAUDE.md — Agent Coding Instructions

> Full project docs in README.md. This file is the compact reference for coding.

## Quick Ref

- **SDK**: nRF Connect SDK v3.3.0 / Zephyr v4.3.99
- **Module**: Raytac MDBT50Q-P1MV2 (nRF52840)
- **Board**: `cpap_pi_control/nrf52840` (custom, in `Boards/kamoamoa/cpap_pi_control/`)
- **Build**: `cd build && ninja` (app image in `build/CPAP_PI_firmware/`)
- **Working dir**: `c:\Users\xwang3239\Downloads\CPAP_PI_firmware` (the old `Moamoa_CPAP_PI_firmware` checkout is retired)
- **Ninja**: `C:\Users\xwang3239\ncs\toolchains\936afb6332\opt\bin\ninja.exe`
- **Console**: RTT only (`printk()`), no UART. No `%f` — use fixed-point ints. Enable float-printing when necessary

## Commands:
Load nrf connect sdk terminal for env variables, etc, please do this before any command below other than the RTT reading:
$ nrfutil sdk-manager toolchain launch --ncs-version v3.3.0 --terminal
Build project (the `-- -DBOARD_ROOT=.` is required on the first/pristine configure so sysbuild finds the custom board):
$ west build -b cpap_pi_control/nrf52840 -- -DBOARD_ROOT=.
Flash firmware after a successful build:
$ west flash --recover
Load RTT console and connect to the board (makesure to set a timeout of 5 seconds, if there's nothing or returned meaning no output):
$ & "~\Downloads\SimplicityCommander-Windows\SimplicityCommander-Windows\Commander-cli_win32_x64_1v24p1b1980\Simplicity Commander CLI\commander-cli.exe"  rtt connect --device nrf52840_M4

## Pin Map

| Pin   | Function          | Notes                              |
|-------|-------------------|------------------------------------|
| P0.02 | AIN0 (in)         | FF1 — ESS102 FSR 1 via OPA2333     |
| P0.03 | AIN1 (in)         | FF2 — ESS102 FSR 2 via OPA2333     |
| P0.04 | AIN2 (in)         | FF3 — ESS102 FSR 3 via OPA2333     |
| P0.05 | AIN3 (in)         | VREF — op-amp reference rail       |
| P0.07 | CS1 (out)         | MS5611 baro 1 chip-select          |
| P0.08 | I2C SDA           | To TCA9548A mux                    |
| P0.11 | PPG_INT (in)      | MAX30101 interrupt (shared)        |
| P0.12 | I2C SCL           | To TCA9548A mux                    |
| P0.13 | SPI SCK (out)     | To mask flex via 30-pin FPC        |
| P0.14 | SPI MOSI (out)    | To mask flex via 30-pin FPC        |
| P0.17 | CS2 (out)         | MS5611 baro 2 chip-select          |
| P0.18 | nRESET            | gpio-as-nreset                     |
| P0.19 | CS3 (out)         | MS5611 baro 3 chip-select          |
| P0.20 | CS4 (out)         | MS5611 baro 4 chip-select          |
| P0.21 | SPI MISO (in)     | From mask flex via 30-pin FPC      |
| P0.22 | CS5 (out)         | MS5611 baro 5 chip-select          |
| P0.24 | CS6 (out)         | MS5611 baro 6 chip-select          |
| P1.00 | TCA_RST (out)     | TCA9548A reset, active-low         |
| P1.08 | LED1 (out)        | Active-low                         |
| P1.09 | LED2 (out)        | Active-low                         |

## Sensors on the mask flex PCB (via 30-pin FPC J1/J5)

- 3× **MAX30101** PPG @ I2C `0x57`, one per mux channel ch0–ch2, shared PPG_INT
- 1× **SHT40** temp/humidity @ I2C `0x44`, mux ch3
- 6× **MS5611** barometric pressure, SPI mode, one per chip-select CS1–CS6
- 3× **ESS102** force sensors (FSR tails FF1c–FF3c) → OPA2333 TIA on control board → AIN0–AIN2

## I2C Addresses (7-bit)

- TCA9548A (mux): `0x70` (A0=A1=A2=GND)
- MAX30101 ×3: `0x57` (behind mux ch0–ch2)
- SHT40: `0x44` (behind mux ch3)

## Sampling rates (see README for full budget analysis)

- PPG: 100 Hz/sensor (3-LED, 411 µs PW; ceiling ~200 Hz) — burst-drain FIFOs every 20–40 ms
- FSR: 100 Hz (match PPG loop; SAADC is nowhere near limiting)
- MS5611: 100 Hz all six at OSR 2048 (each tick: read previous conversion, start next; T at 1 Hz interleaved). Ceiling ~200 Hz at OSR 1024
- SHT40: 1 Hz high-precision (self-heating — keep duty low)
- BLE: binary-packed stream ≈ 5–6 kB/s fits NUS + DLE (20–40 kB/s practical; Web Bluetooth portals often only 10–20 kB/s). Batch 25–50 ms per notification. JSON @100 Hz does NOT fit — debug mode only.

## SOC components to use
i2c0 (TWIM0, to mux), spi1 (SPIM1, 6 cs-gpios), adc (SAADC AIN0-3), bunch of gpios.
32.768 kHz crystal fitted → K32SRC_XTAL. NOTE: i2c0 must not be used together with spi0, and i2c1 not with spi1 (shared peripheral IDs).

## Code map (see README for details)

- `src/sensors/sensor_manager.c` — 10 ms sampling thread: PPG+FSR 100 Hz, baro 25 Hz, SHT 1 Hz; prints 1 Hz RTT summary; feeds comm layer per tick
- `src/comm/comm_protocol.h` — binary frame spec (DATA 224 B @25/s, STATUS 41 B @1/s); portal parser AND scripts/rtt_bridge.py FRAME_LEN must match this file
- `src/comm/` — NUS transport (ble_manager) + frame batching (comm_manager); NUS RX: 'B' binary / 'J' JSON debug
- `src/comm/rtt_stream.c` — same frames on RTT up-buffer 1, always on; PC side: `python scripts/rtt_bridge.py` → ws://localhost:8765 (stop other RTT sessions first)
- `src/drivers/driver_ms5611.c` — MS5611 math (NOT ms5607 exponents); concurrent conversions
- `Boards/.../board.c` — REGOUT0=3V3 UICR hook (high-voltage mode, do not remove; keep REG0 in LDO — no HV DCDC, L4 unpopulated)
- Flash with plain `west flash`; use `--recover` only for locked/wedged chips (wipes UICR + future stored data)
- Web portal counterpart: `../CPAP_PI_portal` (binary frame parser in src/useComm.js)
