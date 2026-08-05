#ifndef LORA_MODULE_H
#define LORA_MODULE_H

#include <Arduino.h>

class LoraModule {
public:
    LoraModule(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin, uint8_t m0Pin, uint8_t m1Pin)
        : _serial(serial), _rxPin(rxPin), _txPin(txPin), _m0Pin(m0Pin), _m1Pin(m1Pin) {}

    bool begin(uint32_t frequency, uint8_t sck = 0, uint8_t miso = 0, uint8_t mosi = 0) {
        // Set mode pins to Normal Mode (M0=0, M1=0)
        pinMode(_m0Pin, OUTPUT);
        pinMode(_m1Pin, OUTPUT);
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, LOW);

        // Wait for pins to stabilize
        delay(10);

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
    uint8_t _m0Pin;
    uint8_t _m1Pin;
};

#endif
