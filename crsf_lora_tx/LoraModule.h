#ifndef LORA_MODULE_H
#define LORA_MODULE_H

#include <Arduino.h>

class LoraModule {
public:
    LoraModule(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin)
        : _serial(serial), _rxPin(rxPin), _txPin(txPin) {}

    bool begin(uint32_t frequency, uint8_t sck = 0, uint8_t miso = 0, uint8_t mosi = 0) {
        // Initialize pre-instantiated global HardwareSerial reference for Ebyte E32
        _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
        return true;
    }

    bool sendPacket(const uint8_t *buf, uint8_t size) {
        size_t written = _serial.write(buf, size);
        _serial.flush();
        return (written == size);
    }

    void startReceive() {
        // Transmitter does not receive, but we keep this empty for API compatibility
    }

    int parsePacket() {
        return 0;
    }

    int readPacket(uint8_t *buf, uint8_t maxLen) {
        return 0;
    }

private:
    HardwareSerial &_serial;
    uint8_t _rxPin;
    uint8_t _txPin;
};

#endif
