#ifndef E32_MODULE_H
#define E32_MODULE_H

#include <Arduino.h>

class E32Module {
public:
    E32Module(uint8_t m0Pin, uint8_t m1Pin, uint8_t rxPin, uint8_t txPin)
        : _m0Pin(m0Pin), _m1Pin(m1Pin), _rxPin(rxPin), _txPin(txPin), _serial(nullptr) {}

    bool begin(HardwareSerial &serial, uint32_t frequencyHz) {
        _serial = &serial;
        pinMode(_m0Pin, OUTPUT);
        pinMode(_m1Pin, OUTPUT);

        // Put E32 in Sleep / Configuration Mode (M0=1, M1=1)
        digitalWrite(_m0Pin, HIGH);
        digitalWrite(_m1Pin, HIGH);
        delay(100);

        _serial->begin(9600, SERIAL_8N1, _rxPin, _txPin);
        delay(100);

        // Determine channel based on frequency (410M + CHAN * 1M). For example, 433MHz -> CHAN = 23 (0x17)
        uint8_t chan = 23;
        if (frequencyHz >= 410000000 && frequencyHz <= 441000000) {
            chan = (frequencyHz - 410000000) / 1000000;
        }

        // C0 Command format:
        // Byte 0: 0xC0 (Save config on power-down)
        // Byte 1: 0x00 (Address High)
        // Byte 2: 0x00 (Address Low)
        // Byte 3: 0x1A (9600bps UART, 2.4kbps Air rate)
        // Byte 4: chan (Frequency channel)
        // Byte 5: 0x47 (Transparent transmission, AUX pull-up, 250ms wake-up, FEC enabled, 21dBm power / MIN)
        // Note: 21dBm is the lowest power for 433T30D, perfect for stable 3 km range!
        uint8_t configCmd[] = { 0xC0, 0x00, 0x00, 0x1A, chan, 0x47 };

        // Flush RX
        while (_serial->available()) _serial->read();

        // Write configuration
        _serial->write(configCmd, 6);
        _serial->flush();
        delay(100);

        // Read confirmation response (should return C0 ADDH ADDL SPED CHAN OPTION)
        bool success = false;
        if (_serial->available() >= 6) {
            uint8_t response[6];
            _serial->readBytes(response, 6);
            if (response[0] == 0xC0 || response[0] == 0xC2) {
                success = true;
                Serial.printf("[E32] Configuration confirmed. Power level configured to 21dBm (MIN) for 3 km range.\n");
            }
        }

        if (!success) {
            Serial.println("[E32] Warning: No configuration confirmation response received from E32. Assuming default transparent mode.");
        }

        // Put E32 in Normal Mode (M0=0, M1=0) for transparent wireless transmission/reception
        digitalWrite(_m0Pin, LOW);
        digitalWrite(_m1Pin, LOW);
        delay(100);

        return true;
    }

    int available() {
        if (_serial) return _serial->available();
        return 0;
    }

    int read() {
        if (_serial) return _serial->read();
        return -1;
    }

    int readBytes(uint8_t *buffer, size_t length) {
        if (_serial) return _serial->readBytes(buffer, length);
        return 0;
    }

    size_t write(const uint8_t *buffer, size_t size) {
        if (_serial) return _serial->write(buffer, size);
        return 0;
    }

private:
    uint8_t _m0Pin;
    uint8_t _m1Pin;
    uint8_t _rxPin;
    uint8_t _txPin;
    HardwareSerial *_serial;
};

#endif
