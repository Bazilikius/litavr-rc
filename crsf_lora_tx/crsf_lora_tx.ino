/*
 * CRSF to Ebyte E32 LoRa Transmitter for ESP32 (Dev Module)
 * - Listens to CRSF stream on Serial2 (RX Pin 16, TX Pin 17 is unused) at CRSF baudrate (default 400000).
 * - Packs 16 channels and broadcasts them transparently over Ebyte E32 UART LoRa module on Serial1.
 * - Restricts WiFi TX Power to 25% for high efficiency and compliance.
 * - Ebyte E32 UART LoRa connections: RX1 = GPIO 21, TX1 = GPIO 22, M0 = GPIO 5, M1 = GPIO 18.
 * - Power level configured to 21dBm (MIN), perfect for highly stable 3 km range!
 */

#include <Arduino.h>
#include <WiFi.h>
#include "CrsfParser.h"
#include "ConfigManager.h"
#include "E32Module.h"

// Hardware configuration
#define CRSF_SERIAL Serial2
#define CRSF_RX_PIN 16
#define CRSF_TX_PIN -1

// E32 UART LoRa Pin Configuration
#define E32_M0   5
#define E32_M1   18
#define E32_RX   21
#define E32_TX   22

CrsfParser parser;
ConfigManager configManager;
E32Module e32(E32_M0, E32_M1, E32_RX, E32_TX);

uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t loraSentCount = 0;
uint32_t lastReport = 0;

struct LoraPacket {
    uint16_t signature; // 0x55AA
    uint32_t packetId;
    uint16_t channels[16];
} loraPacket;

void setup() {
    // 1. Silent delay to allow external BEC/battery power rails to stabilize completely before starting up
    delay(1000);

    Serial.begin(115200);
    delay(2000); // 2-second safe boot delay

    Serial.println("\n=============================================");
    Serial.println(" ESP32 CRSF to E32 LoRa TRANSMITTER ");
    Serial.println("=============================================");

    // Load configuration
    configManager.begin();
    TxConfig activeConfig = configManager.getConfig();

    // Start WiFi in AP mode with reduced TX power to 25%
    WiFi.softAP("CRSF-TX-Config", "12345678");
    WiFi.setTxPower(WIFI_POWER_5dBm); // ~25% WiFi TX power
    Serial.println("WiFi Access Point 'CRSF-TX-Config' started.");
    Serial.printf("WiFi Transmit Power limited to 5dBm (25%% power).\n");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Initialize Ebyte E32 UART LoRa Module
    Serial.printf("[E32] Initializing Ebyte E32 at frequency %u Hz...\n", activeConfig.loraFreq);
    e32.begin(Serial1, activeConfig.loraFreq);

    // Initialize CRSF Hardware Serial
    Serial.printf("Initializing CRSF on Serial2 at %u baud (RX=%d)...\n", activeConfig.crsfBaudrate, CRSF_RX_PIN);
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

            // Broadcast transparently via Ebyte E32
            e32.write((uint8_t*)&loraPacket, sizeof(loraPacket));
            loraSentCount++;
        }
    }

    // Yield to background tasks
    delay(1);

    // Diagnostics every 5 seconds
    if (millis() - lastReport > 5000) {
        TxConfig activeConfig = configManager.getConfig();
        Serial.printf("[E32 TX] Bytes: %u | CRSF Packets: %u | E32 Sent: %u | Freq: %u MHz\n",
                      byteCount, packetCount, loraSentCount, activeConfig.loraFreq / 1000000);
        lastReport = millis();
    }
}
