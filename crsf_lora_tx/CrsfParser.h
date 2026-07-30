#ifndef CRSF_PARSER_H
#define CRSF_PARSER_H

#include <Arduino.h>

#define CRSF_MAX_PACKET_LEN 64
#define CRSF_PAYLOAD_OFFSET 3

class CrsfParser {
public:
    CrsfParser();
    bool processByte(uint8_t byte);
    uint16_t getChannel(uint8_t index); // 0-indexed

private:
    uint8_t _buffer[CRSF_MAX_PACKET_LEN];
    uint8_t _bufferIndex;
    uint16_t _channels[16];

    uint8_t _crc8(const uint8_t *ptr, uint8_t len);
    void _unpackChannels(const uint8_t *payload);
};

#endif
