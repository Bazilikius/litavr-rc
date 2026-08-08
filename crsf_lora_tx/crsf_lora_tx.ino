/*
 * CRSF to LoRa Transmitter for ESP32 (Dev Module)
 * - Listens to CRSF stream on Serial1 (RX Pin 13 is safe, TX Pin -1 is unused) at CRSF baudrate (default 400000).
 * - Packs 16 channels and broadcasts them over Ebyte E32 UART LoRa module on Serial2 (RX=18, TX=19).
 * - Restricts WiFi TX Power to 25% for high efficiency and compliance.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "CrsfParser.h"
#include "ConfigManager.h"
#include "LoraModule.h"
#include "WebServerHandler.h"

// Hardware configuration for CRSF (using Serial1 on safe, non-conflicting GPIO 13)
#define CRSF_SERIAL Serial1
#define CRSF_RX_PIN 13
#define CRSF_TX_PIN -1

// Ebyte E32 UART Pin Configuration on Serial2 (RX=18, TX=19)
#define E32_RX_PIN 18  // Connected to E32 TXD
#define E32_TX_PIN 19  // Connected to E32 RXD

CrsfParser parser;
ConfigManager configManager;
LoraModule lora(Serial2, E32_RX_PIN, E32_TX_PIN);

uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t loraSentCount = 0;

struct __attribute__((packed)) LoraPacket {
    uint16_t signature; // 0x55AA
    uint32_t packetId;
    uint16_t channels[16];
} loraPacket;

WebServerHandler webServer(configManager, byteCount, packetCount, loraSentCount);

bool wifiShutDownDone = false;

void setup() {
    // 1. Silent delay to allow external BEC/battery power rails to stabilize completely before starting up
    delay(3000); // Expanded boot delay to be safe and silent

    // Load configuration
    configManager.begin();
    TxConfig activeConfig = configManager.getConfig();

    // Start WiFi in AP mode with reduced TX power to 25% and begin Web Server
    webServer.begin();

    // Initialize Ebyte E32 UART LoRa Module
    if (lora.begin(activeConfig.loraFreq)) {
        // Init successful
    }

    // Initialize CRSF Hardware Serial (using GPIO 13 to avoid PSRAM GPIO 16/17 conflict)
    CRSF_SERIAL.begin(activeConfig.crsfBaudrate, SERIAL_8N1, CRSF_RX_PIN, CRSF_TX_PIN);

    loraPacket.signature = 0x55AA;
    loraPacket.packetId = 0;
}

void loop() {
    // 1. Automatic WiFi shutdown after 3 minutes (180,000 ms)
    if (!wifiShutDownDone && millis() > 180000) {
        WiFi.mode(WIFI_OFF);
        wifiShutDownDone = true;
    }

    // 2. Handle Web requests (if WiFi is still on)
    if (!wifiShutDownDone) {
        webServer.handleClient();
    }

    // 3. Parse incoming CRSF stream
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
