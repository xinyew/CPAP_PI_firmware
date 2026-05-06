#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>
#include <string.h>

#include "sensors/max30101/max30101_reader.h"
#include "comm/comm_manager.h"
#include "MAX30105.h" // EmotiBit Library Header

LOG_MODULE_REGISTER(max30101_reader, LOG_LEVEL_WRN);

MAX30105 particleSensor;

extern "C" {

static int max30101_init(void)
{
    LOG_INF("Initializing MAX30101 using EmotiBit Library...");

    // Initialize sensor
    if (!particleSensor.begin(Wire0, I2C_SPEED_STANDARD, MAX30105_ADDRESS)) {
        LOG_ERR("MAX30101 was not found. Please check wiring/power.");
        return -ENODEV;
    }

    LOG_INF("MAX30101 found! Configuring...");

    // Setup to use EmotiBit default settings (or customize)
    // setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange)
    byte powerLevel = 0x1F; // ~6.4mA
    byte sampleAverage = 1; // No averaging (maximum frequency)
    byte ledMode = 3;       // Red + IR + Green (Multi-LED)
    int sampleRate = 100;    // 100 Hz
    int pulseWidth = 411;   // 18-bit resolution
    int adcRange = 4096;    // 15.63 pA/LSB

    particleSensor.setup(powerLevel, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

    LOG_INF("MAX30101 successfully configured with EmotiBit algorithms.");
    return 0;
}

static int max30101_read(void)
{
    // The MAX30101 has an internal FIFO that buffers readings.
    particleSensor.check();

    // While there are new samples available in the library's buffer
    while (particleSensor.available()) {
        current_sensor_data.ppg_red = particleSensor.getFIFORed();
        current_sensor_data.ppg_ir = particleSensor.getFIFOIR();
        current_sensor_data.ppg_green = particleSensor.getFIFOGreen();

        // Advance the read pointer to the next sample
        particleSensor.nextSample();
    }

    return 0;
}

static uint32_t max30101_get_interval(void)
{
    // Poll the sensor 100 times per second (10ms). 
    // This rapidly drains the FIFO into the current_sensor_data struct.
    return 10;
}

sensor_interface_t max30101_sensor = {
    .name = "MAX30101",
    .init = max30101_init,
    .read = max30101_read,
    .get_interval_ms = max30101_get_interval,
};

} // extern "C"
