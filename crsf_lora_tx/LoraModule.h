#ifndef LORA_MODULE_H
#define LORA_MODULE_H

#include <Arduino.h>
#include <SPI.h>

// Standard SX127x Registers
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_FIFO_ADDR_PTR        0x0d
#define REG_FIFO_TX_BASE_ADDR    0x0e
#define REG_FIFO_RX_BASE_ADDR    0x0f
#define REG_RX_NB_BYTES          0x13
#define REG_FIFO_RX_CURRENT_ADDR 0x10
#define REG_IRQ_FLAGS            0x12
#define REG_MODEM_CONFIG_1       0x1d
#define REG_MODEM_CONFIG_2       0x1e
#define REG_PREAMBLE_MSB         0x20
#define REG_PREAMBLE_LSB         0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_SYNC_WORD            0x39
#define REG_DIO_MAPPING_1        0x40
#define REG_VERSION              0x42

// Modes
#define MODE_LONG_RANGE_MODE     0x80
#define MODE_SLEEP               0x00
#define MODE_STANDBY             0x01
#define MODE_TX                  0x03
#define MODE_RX_CONTINUOUS       0x05

// IRQ flags
#define IRQ_TX_DONE_MASK         0x08
#define IRQ_RX_DONE_MASK         0x40
#define IRQ_PAYLOAD_CRC_ERROR_MASK 0x20

class LoraModule {
public:
    LoraModule(uint8_t ssPin, uint8_t rstPin, uint8_t dio0Pin)
        : _ssPin(ssPin), _rstPin(rstPin), _dio0Pin(dio0Pin) {}

    bool begin(uint32_t frequency, uint8_t sck = 18, uint8_t miso = 19, uint8_t mosi = 23) {
        pinMode(_ssPin, OUTPUT);
        digitalWrite(_ssPin, HIGH);

        if (_rstPin != 255) {
            pinMode(_rstPin, OUTPUT);
            digitalWrite(_rstPin, HIGH);
            // Reset SX127x
            digitalWrite(_rstPin, LOW);
            delay(10);
            digitalWrite(_rstPin, HIGH);
            delay(10);
        }

        SPI.begin(sck, miso, mosi, _ssPin);

        // Check Version
        uint8_t version = readRegister(REG_VERSION);
        if (version != 0x12) {
            Serial.printf("LoRa detection failed. Version: 0x%02X (expected 0x12)\n", version);
            return false;
        }

        // Put in sleep mode to activate LoRa mode
        sleep();

        // Configure Frequency
        setFrequency(frequency);

        // Configure default modem settings: SF7, 125kHz bandwidth, 4/5 coding rate, explicit header
        writeRegister(REG_MODEM_CONFIG_1, 0x72); // 125 kHz, 4/5 coding rate, explicit header
        writeRegister(REG_MODEM_CONFIG_2, 0x74); // SF7, normal TX, CRC enabled

        // Set Preamble length (8 symbols)
        writeRegister(REG_PREAMBLE_MSB, 0);
        writeRegister(REG_PREAMBLE_LSB, 8);

        // Set Sync Word
        writeRegister(REG_SYNC_WORD, 0x12);

        // Configure PA (High power boost enabled)
        writeRegister(REG_PA_CONFIG, 0xFF); // Max Power

        // Set Base Addresses
        writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
        writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

        idle();
        return true;
    }

    void sleep() {
        writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
    }

    void idle() {
        writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STANDBY);
    }

    void setFrequency(uint32_t frequency) {
        uint64_t frf = ((uint64_t)frequency << 19) / 32000000;
        writeRegister(REG_FRF_MSB, (frf >> 16) & 0xFF);
        writeRegister(REG_FRF_MID, (frf >> 8) & 0xFF);
        writeRegister(REG_FRF_LSB, frf & 0xFF);
    }

    bool sendPacket(const uint8_t *buf, uint8_t size) {
        idle();

        // Reset FIFO pointer
        writeRegister(REG_FIFO_ADDR_PTR, 0);
        writeRegister(REG_PAYLOAD_LENGTH, size);

        // Write packet payload
        digitalWrite(_ssPin, LOW);
        SPI.transfer(REG_FIFO | 0x80);
        for (uint8_t i = 0; i < size; i++) {
            SPI.transfer(buf[i]);
        }
        digitalWrite(_ssPin, HIGH);

        // Start TX
        writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

        // Wait for TX Done with a 100ms timeout
        uint32_t start = millis();
        while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0) {
            if (millis() - start > 100) {
                idle();
                return false;
            }
            yield();
        }

        // Clear TX done flag
        writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
        idle();
        return true;
    }

    void startReceive() {
        writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
    }

    int parsePacket() {
        uint8_t irqFlags = readRegister(REG_IRQ_FLAGS);
        // Clear IRQ flags
        writeRegister(REG_IRQ_FLAGS, irqFlags);

        if ((irqFlags & IRQ_RX_DONE_MASK) && !(irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK)) {
            // Valid packet received
            return readRegister(REG_RX_NB_BYTES);
        }
        return 0;
    }

    int readPacket(uint8_t *buf, uint8_t maxLen) {
        uint8_t len = readRegister(REG_RX_NB_BYTES);
        if (len > maxLen) len = maxLen;

        // Set FIFO address to current RX FIFO address
        uint8_t currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
        writeRegister(REG_FIFO_ADDR_PTR, currentAddr);

        digitalWrite(_ssPin, LOW);
        SPI.transfer(REG_FIFO & 0x7F);
        for (uint8_t i = 0; i < len; i++) {
            buf[i] = SPI.transfer(0x00);
        }
        digitalWrite(_ssPin, HIGH);

        return len;
    }

    uint8_t readRegister(uint8_t reg) {
        digitalWrite(_ssPin, LOW);
        SPI.transfer(reg & 0x7F);
        uint8_t val = SPI.transfer(0x00);
        digitalWrite(_ssPin, HIGH);
        return val;
    }

    void writeRegister(uint8_t reg, uint8_t val) {
        digitalWrite(_ssPin, LOW);
        SPI.transfer(reg | 0x80);
        SPI.transfer(val);
        digitalWrite(_ssPin, HIGH);
    }

private:
    uint8_t _ssPin;
    uint8_t _rstPin;
    uint8_t _dio0Pin;
};

#endif
