/*
 * CRSF to LoRa Transmitter for ESP32 (Dev Module)
 * - Listens to CRSF stream on Serial2 (RX Pin 16, TX Pin 17 is unused) at CRSF baudrate (default 400000).
 * - Packs 16 channels and broadcasts them over Ebyte E32 UART LoRa module.
 * - Restricts WiFi TX Power to 25% for high efficiency and compliance.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "CrsfParser.h"
#include "ConfigManager.h"
#include "LoraModule.h"

// Hardware configuration for CRSF
#define CRSF_SERIAL Serial2
#define CRSF_RX_PIN 16
#define CRSF_TX_PIN -1

// Ebyte E32 UART & Mode control Pin Configuration
#define E32_RX_PIN 18  // Connected to E32 TXD
#define E32_TX_PIN 19  // Connected to E32 RXD
#define E32_M0_PIN 14  // Connected to E32 M0
#define E32_M1_PIN 21  // Connected to E32 M1

CrsfParser parser;
ConfigManager configManager;
LoraModule lora(E32_RX_PIN, E32_TX_PIN, E32_M0_PIN, E32_M1_PIN);

uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t loraSentCount = 0;

struct __attribute__((packed)) LoraPacket {
    uint16_t signature; // 0x55AA
    uint32_t packetId;
    uint16_t channels[16];
} loraPacket;

void setup() {
    // 1. Silent delay to allow external BEC/battery power rails to stabilize completely before starting up
    delay(3000); // Expanded boot delay to be safe and silent

    // Load configuration
    configManager.begin();
    TxConfig activeConfig = configManager.getConfig();

    // Start WiFi in AP mode with reduced TX power to 25%
    WiFi.softAP("CRSF-TX-Config", "12345678");
    WiFi.setTxPower(WIFI_POWER_5dBm); // ~25% WiFi TX power

    // Initialize Ebyte E32 UART LoRa Module
    if (lora.begin(activeConfig.loraFreq)) {
        // Init successful
    }

    // Initialize CRSF Hardware Serial
    CRSF_SERIAL.begin(activeConfig.crsfBaudrate, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);

    loraPacket.signature = 0x55AA;
    loraPacket.packetId = 0;
}

void loop() {
    // Parse incoming CRSF stream
    uint8_t readLimit = 0;
    while (CRSF_SERIAL.available() && readLimit < 128) {
        uint8_t b = CRSF_SERIAL.read();
        byteCount++;
        readLimit++;

        if (parser.processByte(b)) {
            packetCount++;

            // Populate packet with live channels
            loraPacket.packetId++;
            for (int i = 0; i < 16; i++) {
                loraPacket.channels[i] = parser.getChannel(i);
            }

            // Broadcast via Ebyte E32 UART
            if (lora.sendPacket((uint8_t*)&loraPacket, sizeof(loraPacket))) {
                loraSentCount++;
            }
        }
    }

    // Yield to background tasks
    delay(1);
}
