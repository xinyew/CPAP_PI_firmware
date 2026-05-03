#include "Wire.h"
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wire_compat, LOG_LEVEL_INF);

TwoWire Wire0(DEVICE_DT_GET(DT_NODELABEL(mux_i2c0)));
TwoWire Wire1(DEVICE_DT_GET(DT_NODELABEL(mux_i2c1)));

TwoWire::TwoWire(const struct device *dev) : i2c_dev(dev), txLength(0), rxIndex(0), rxLength(0) {}

void TwoWire::begin() {
    // Zephyr I2C is initialized via DeviceTree automatically
}

void TwoWire::setClock(uint32_t clock) {
    // Clock configured in DeviceTree
}

void TwoWire::beginTransmission(uint8_t address) {
    txAddress = address;
    txLength = 0;
}

size_t TwoWire::write(uint8_t data) {
    if (txLength < sizeof(txBuffer)) {
        txBuffer[txLength++] = data;
        return 1;
    }
    return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity) {
    size_t i;
    for (i = 0; i < quantity; i++) {
        if (write(data[i]) == 0) break;
    }
    return i;
}

uint8_t TwoWire::endTransmission(void) {
    return endTransmission(true);
}

uint8_t TwoWire::endTransmission(uint8_t stop) {
    if (stop) {
        if (!device_is_ready(i2c_dev)) return 4;
        
        int err = i2c_write(i2c_dev, txBuffer, txLength, txAddress);
        txLength = 0; // Clear buffer
        if (err) return 2;
        return 0;
    } else {
        // If stop is false (repeated start), we hold the txBuffer.
        // It will be used in the next requestFrom call via i2c_write_read!
        return 0;
    }
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity) {
    return requestFrom(address, quantity, true);
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
    if (!device_is_ready(i2c_dev)) return 0;
    
    if (quantity > sizeof(rxBuffer)) quantity = sizeof(rxBuffer);
    
    int err;
    if (txLength > 0) {
        // We have pending data from an endTransmission(false) call.
        // This means we need to do a write followed by a read (repeated start).
        err = i2c_write_read(i2c_dev, address, txBuffer, txLength, rxBuffer, quantity);
        txLength = 0; // Clear pending write
    } else {
        // Just a standard read
        err = i2c_read(i2c_dev, rxBuffer, quantity, address);
    }
    
    if (err) return 0;
    
    rxIndex = 0;
    rxLength = quantity;
    return quantity;
}

int TwoWire::available(void) {
    return rxLength - rxIndex;
}

int TwoWire::read(void) {
    if (rxIndex < rxLength) {
        return rxBuffer[rxIndex++];
    }
    return -1;
}
