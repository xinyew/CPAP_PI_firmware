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

## Sampling-Rate Budget (per-sensor analysis)

Target profile for the live web-portal plots, derived from signal physics and
the measured bus topology (3× MAX30101 behind one 400 kHz I2C mux, 6× MS5611
sharing one SPI bus, FSRs on the SAADC):

| Sensor | Meaningful rate | Achievable ceiling | Reason for the ceiling |
|---|---|---|---|
| PPG (MAX30101 ×3) | **100 Hz** each | ~200 Hz each | Sensor-side: 3-LED mode needs the 411 µs pulse width for SNR; I2C load at 100 Hz is only ~2.7 kB/s (<10% of the 400 kHz bus incl. mux switches) |
| FSR (ESS102 ×3 + VREF) | **100 Hz** (match PPG) | ~1 kHz+ | SAADC is 200 kSPS; signal content (respiration, mask shifts) is only a few Hz — faster sampling just plots noise |
| Baro (MS5611 ×6) | **25–50 Hz** all six | ~100 Hz all six (P-only, T at 1 Hz); ~200 Hz at OSR 1024 | Conversion time (9.04 ms at OSR 4096), NOT SPI — all six chips convert simultaneously. Prefer high OSR (0.012 mbar RMS) over speed for contact pressure |
| SHT40 | **1 Hz** high-precision | — | Micro-climate drifts over seconds; hard polling self-heats the sensor |

Notes:
- Drain PPG FIFOs in bursts every 20–40 ms; mux channel switch ≈ 50 µs amortizes to nothing.
- MS5611: command all six, wait one conversion, read all six. Run temperature conversions at 1 Hz interleaved with pressure.
- CPU (64 MHz M4F) and RAM are not limiting; all buses run on EasyDMA.

## BLE Throughput Budget

- Recommended profile above, **binary-packed** (3 B per 18-bit PPG channel,
  int16 mV force, int24 pressure) ≈ **5–6 kB/s (~48 kbps)**.
- NUS link with MTU 247 + DLE at 1M PHY realistically moves 20–40 kB/s
  (more on 2M PHY) → 3–6× headroom, **sensor rates need not drop for BLE**,
  provided samples are batched ~25–50 ms per notification to fill 244-byte
  payloads.
- **JSON does not scale**: ~250–300 B/frame × 100 Hz ≈ 25–30 kB/s of ASCII —
  over budget. Keep JSON only as a low-rate debug mode.
- Web-Bluetooth caveat: browsers can't request short connection intervals;
  OS stacks often settle at 15–30 ms → practical portal throughput is
  commonly 10–20 kB/s. Binary framing stays inside even the pessimistic end.
- Decimate the *plot* to ~30 fps regardless; log the full rate, draw less.

## BLE Stream Protocol

Transport: **Nordic UART Service (NUS)**, advertising as `CPAP_PI_Control`
(the web portal matches the `CPAP` name prefix). Full byte-level frame
layout lives in `src/comm/comm_protocol.h`; summary:

- **DATA frame** (176 B, 25/s): 4 batched 10 ms ticks — 3× PPG (R/IR/G
  as u24 each), 4× FSR sample sets (i16 mV), 6× baro pressure (i32 Pa),
  plus validity masks and a millisecond timestamp.
- **STATUS frame** (41 B, 1/s): SHT40 temp/RH, 6× baro temperature,
  3× FSR resistance, achieved rates, validity masks.
- **Commands** (NUS RX, single byte): `B` = binary streaming (default),
  `J` = 1 Hz eval-style JSON debug line.

Requires ATT MTU ≥ 179 (browsers negotiate 247+; prj.conf enables
DLE 251 / MTU 247). Web portal lives in `../CPAP_PI_portal`.

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
