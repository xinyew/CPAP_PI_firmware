#ifndef WIRE_COMPAT_H
#define WIRE_COMPAT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus

class TwoWire {
public:
    TwoWire();
    
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
    uint8_t txAddress;
    uint8_t txBuffer[32];
    size_t txLength;
    
    uint8_t rxBuffer[32];
    size_t rxIndex;
    size_t rxLength;
};

extern TwoWire Wire;

#endif /* __cplusplus */

#endif /* WIRE_COMPAT_H */
