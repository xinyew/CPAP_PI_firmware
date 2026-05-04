# CPAP Pressure Injury Sensing System
## Hardware Circuit Schematic

This document outlines the physical hardware connections required to successfully run the firmware.

### Schematic Block Diagram

```mermaid
graph TD
    %% Define components
    NRF["nRF52840 DK (Microcontroller)"]
    MUX["TCA9548A (I2C Multiplexer)"]
    MAX["MAX30101 (Pulse Oximeter)"]
    SHT["SHT40 (Temp & Humidity)"]
    ESS["ESS102 (Force Sensor)"]

    %% Define logical sub-groups
    subgraph I2C_Bus ["I2C Bus Architecture"]
        MUX
        MAX
        SHT
    end

    subgraph Analog_Input ["Differential Analog Input"]
        ESS
    end

    %% nRF to MUX connections
    NRF -- "P0.27 (SDA)" --> MUX
    NRF -- "P0.26 (SCL)" --> MUX
    NRF -- "P0.02 (Reset, Active Low)" --> MUX
    NRF -- "P1.11 (A0 = GND)" --> MUX
    NRF -- "P1.15 (A1 = GND)" --> MUX
    NRF -- "P1.14 (A2 = GND)" --> MUX

    %% MUX to I2C Sensors
    MUX -- "Channel 0 (SDA0/SCL0)" --> MAX
    MUX -- "Channel 1 (SDA1/SCL1)" --> SHT

    %% Additional Sensor Pins
    MAX -- "P1.13 (INT, Active Low)" --> NRF

    %% Analog Sensor Pins
    ESS -- "P0.03 (AIN1, Positive Signal)" --> NRF
    ESS -- "P0.05 (AIN3, 0.3V Reference)" --> NRF
```

### Important Wiring Notes:
1. **MUX Addressing**: The `A0`, `A1`, and `A2` pins on the TCA9548A must be driven LOW to set the MUX I2C address to `0x70`. The firmware forces `P1.11`, `P1.15`, and `P1.14` LOW automatically on boot.
2. **ESS102 Force Sensor**: Because the force sensor signal voltage can potentially fall slightly below the 0.3V reference, it MUST be wired differentially. 
   - Wire your `0.3V` reference directly to `P0.05`. 
   - Wire your sensor output directly to `P0.03`. 
   - The nRF52840 will safely subtract `P0.05` from `P0.03` mathematically. Do not attach negative voltages relative to Ground to any pin.
3. **Power**: Provide 3.3V power (VDD) and Common Ground (GND) to the TCA9548A, MAX30101, and SHT40 breakout boards from the nRF52840 DK.
