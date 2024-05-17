
#ifndef sfe_bus_h
#define sfe_bus_h

// sfe_bus.h

    bool SfeI2C_init(uint8_t address);

    bool SfeI2C_ping(void);

    uint16_t SfeI2C_available(void);

    uint8_t SfeI2C_writeBytes(uint8_t *data, uint8_t length);

    uint8_t SfeI2C_readBytes(uint8_t *data, uint8_t length);


#endif
