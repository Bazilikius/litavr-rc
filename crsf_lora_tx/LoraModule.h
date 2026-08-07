#ifndef LORA_MODULE_H
#define LORA_MODULE_H

#include <Arduino.h>

class LoraModule {
public:
    LoraModule(HardwareSerial &serial, uint8_t rxPin, uint8_t txPin, uint8_t m0Pin, uint8_t m1Pin)
        : _serial(serial), _rxPin(rxPin), _txPin(txPin), _m0Pin(m0Pin), _m1Pin(m1Pin) {}

    bool begin(uint32_t frequency, uint8_t sck = 0, uint8_t miso = 0, uint8_t mosi = 0) {
        // Initialize pre-instantiated global HardwareSerial reference for Ebyte E32
        _serial.begin(9600, SERIAL_8N1, _rxPin, _txPin);

        // Put module in Mode 3 (Configuration Mode) by setting M0=1, M1=1
        pinMode(_m0Pin, OUTPUT);
        pinMode(_m1Pin, OUTPUT);
        digitalWrite(_m0Pin, HIGH);
        digitalWrite(_m1Pin, HIGH);

        // Wait for E32 to switch modes
        delay(100);

        // Clear any old garbage in buffer
        while (_serial.available()) {
            _serial.read();
        }

        // Calculate channel byte based on frequency
        uint8_t chan = 23; // Default to 433MHz (channel 23: 410 + 23 = 433MHz)
        if (frequency >= 862000000 && frequency <= 893000000) {
            chan = (frequency - 862000000) / 1000000;
        } else if (frequency >= 900000000 && frequency <= 931000000) {
            chan = (frequency - 900000000) / 1000000;
        } else if (frequency >= 410000000 && frequency <= 441000000) {
            chan = (frequency - 410000000) / 1000000;
        }

        // 6-byte Ebyte E32 Configuration Command
        // Byte 0: 0xC0 (Write parameters persistently)
        // Byte 1: ADDH = 0x00 (Default Address High)
        // Byte 2: ADDL = 0x00 (Default Address Low)
        // Byte 3: SPED = 0x1A (8N1 Parity, 9600 baud, 2.4kbps air data rate)
        // Byte 4: CHAN = calculated chan (Frequency mapping)
        // Byte 5: OPTION = 0x44 (Transparent transceive mode, Max Tx power 30dBm / 1W)
        uint8_t cmd[6] = { 0xC0, 0x00, 0x00, 0x1A, chan, 0x44 };

        _serial.write(cmd, 6);
        _serial.flush();

        // Wait for write to complete
        delay(100);

        // Put module in Mode 0 (Normal Mode) by setting M0=0, M1=0
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, LOW);

        // Wait for transceiver to stabilize in Normal Mode
        delay(100);

        // Flush any configuration response echo from buffer
        while (_serial.available()) {
            _serial.read();
        }

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
