# CLAUDE.md - CPAP PI Firmware Dev Guidelines

This document outlines compilation, flashing, and development practices for the CPAP Pressure Injury Sensing Firmware project.

## Build and Flash Commands

This project uses the Nordic **nRF Connect SDK** and **Zephyr RTOS** build systems (`west`).

* **Pristine Rebuild (Recommended when files/configs are added):**
  ```bash
  west build -p always -b nrf52840dk_nrf52840
  ```
* **Incremental Build:**
  ```bash
  west build -b nrf52840dk_nrf52840
  ```
* **Flash Binary to Target (via J-Link/USB):**
  ```bash
  west flash
  ```
* **Clean Build Files:**
  ```bash
  rm -rf build
  ```

---

## Coding Style & Conventions

### Language & Frameworks
* **C99** for the core architecture and sensor interfaces.
* **C++17** for compatibility with custom Arduino C++ wrappers (MAX30101, SHT40).
* **Zephyr APIs**: Always prefer native Zephyr kernel, I2C, ADC, and GPIO APIs over custom hardware abstractions where applicable.

### Naming Conventions
* **C Sources (sensor_manager, ms5611, etc.):** Use `snake_case` for functions, variables, and file names. Example: `sensor_manager_init()`.
* **C++ Classes (MAX30101, SHT40):** Use `CamelCase` or standard Arduino APIs to preserve library compatibility.
* **Constants & Macros:** All uppercase with underscores. Example: `NUM_SENSORS`.

### DeviceTree and Kconfig Configuration
* Do not hardcode pin numbers or raw hardware addresses in source code.
* Expose all peripherals, channels, and configuration parameters in **`app.overlay`** (DeviceTree) and refer to them via standard DeviceTree macros (`DT_NODELABEL`, `DT_GET_ANY`, etc.).
* Standard system and stack configurations go in **`prj.conf`**.

### Logging & Error Handling
* Use Zephyr's logging subsystem instead of `printf`:
  ```c
  #include <zephyr/logging/log.h>
  LOG_MODULE_REGISTER(your_module_name, LOG_LEVEL_INF);
  ```
* Return standard Zephyr error codes on failure (e.g., `-ENODEV`, `-EIO`, `-EINVAL`).

### Multiplexer Channel Mapping (TCA9548A)
* **Channel 0 (`mux_i2c0`):** MAX30101 Pulse Oximeter.
* **Channel 1 (`mux_i2c1`):** SHT40 Temperature/Humidity & MS5611 Barometric Pressure Sensor.
