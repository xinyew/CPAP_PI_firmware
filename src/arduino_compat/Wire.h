#ifndef WIRE_COMPAT_H
#define WIRE_COMPAT_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/device.h>

#ifdef __cplusplus

class TwoWire {
public:
    TwoWire(const struct device *dev);
    
    // Config
    void begin();
    void setClock(uint32_t clock);
    
    // Transmission
    void beginTransmission(uint8_t address);
    uint8_t endTransmission(void);
    uint8_t endTransmission(uint8_t stop);
    
    // Writing
    size_t write(uint8_t data);
    size_t write(const uint8_t *data, size_t quantity);
    
    // Reading
    uint8_t requestFrom(uint8_t address, uint8_t quantity);
    uint8_t requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop);
    
    int available(void);
    int read(void);

private:
    const struct device *i2c_dev;
    
    uint8_t txAddress;
    uint8_t txBuffer[32];
    size_t txLength;
    
    uint8_t rxBuffer[32];
    size_t rxIndex;
    size_t rxLength;
};

// Virtual MUX instances
extern TwoWire Wire0; // MUX Channel 0
extern TwoWire Wire1; // MUX Channel 1
extern TwoWire Wire;  // Dummy for legacy default arguments

#endif /* __cplusplus */

#endif /* WIRE_COMPAT_H */
