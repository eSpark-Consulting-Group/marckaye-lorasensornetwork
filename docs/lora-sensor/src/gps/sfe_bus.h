
#ifndef sfe_bus_h
#define sfe_bus_h

// sfe_bus.h

    bool SfeI2C_init(uint8_t address);

    bool SfeI2C_ping();

    uint16_t SfeI2C_available();

    uint8_t SfeI2C_writeBytes(uint8_t *data, uint8_t length);

    uint8_t SfeI2C_writeReadBytes(const uint8_t *data, uint8_t *readData, uint8_t length);

    void SfeI2C_startWriteReadByte(void);
    void SfeI2C_writeReadByte(const uint8_t *data, uint8_t *readData);
    void SfeI2C_writeReadByte2(const uint8_t data, uint8_t *readData);
    void SfeI2C_endWriteReadByte(void);

    uint8_t SfeI2C_readBytes(uint8_t *data, uint8_t length);


#endif
