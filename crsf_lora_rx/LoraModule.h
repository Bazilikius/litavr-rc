#ifndef LORA_MODULE_H
#define LORA_MODULE_H

#include <Arduino.h>

class LoraModule {
public:
    LoraModule(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin)
        : _serial(serial), _rxPin(rxPin), _txPin(txPin), _bufSize(0) {}

    bool begin(uint32_t frequency, uint8_t sck = 0, uint8_t miso = 0, uint8_t mosi = 0) {
        // Initialize pre-instantiated global HardwareSerial reference for Ebyte E32
        _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);
        _bufSize = 0;
        return true;
    }

    bool sendPacket(const uint8_t *buf, uint8_t size) {
        size_t written = _serial.write(buf, size);
        _serial.flush();
        return (written == size);
    }

    void startReceive() {
        // Continuous receive is automatic on Ebyte E32 Normal Mode
    }

    int parsePacket() {
        // Read any available bytes into our sliding window buffer
        while (_serial.available()) {
            uint8_t b = _serial.read();
            if (_bufSize < 38) {
                _buffer[_bufSize++] = b;
            } else {
                // Buffer full, shift left and append
                for (int i = 0; i < 37; i++) {
                    _buffer[i] = _buffer[i + 1];
                }
                _buffer[37] = b;
            }

            // Check if we have a full packet that matches the signature
            while (_bufSize >= 38) {
                // Check signature: 0x55AA (0xAA is first byte, 0x55 is second byte)
                if (_buffer[0] == 0xAA && _buffer[1] == 0x55) {
                    return 38; // Valid packet found!
                } else {
                    // Shift left by 1 byte to find next potential signature
                    for (int i = 0; i < _bufSize - 1; i++) {
                        _buffer[i] = _buffer[i + 1];
                    }
                    _bufSize--;
                }
            }
        }
        return 0;
    }

    int readPacket(uint8_t *buf, uint8_t maxLen) {
        if (_bufSize < 38) return 0;

        uint8_t len = (maxLen < 38) ? maxLen : 38;
        memcpy(buf, _buffer, len);

        // Consume the read packet
        _bufSize = 0;
        return len;
    }

private:
    HardwareSerial &_serial;
    uint8_t _rxPin;
    uint8_t _txPin;

    uint8_t _buffer[38];
    uint8_t _bufSize;
};

#endif
