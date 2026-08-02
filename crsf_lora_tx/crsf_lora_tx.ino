/*
 * CRSF to LoRa Transmitter for ESP32 (Dev Module)
 * - Listens to CRSF stream on Serial2 (RX Pin 16, TX Pin 17 is unused).
 * - Packs 16 channels and broadcasts them over SX127x LoRa Module.
 * - Restricts WiFi TX Power to 25% for high efficiency and compliance.
 * - LORA_DIO0 moved to GPIO 34 (from GPIO 22/2) to prevent ESP32 memory bus or strapping pin conflicts.
 *   GPIO 34 is a safe input-only pin on ESP32, which guarantees zero startup boot conflicts or flash failures!
 */

#include <Arduino.h>
#include <WiFi.h>
#include "CrsfParser.h"
#include "ConfigManager.h"
#include "LoraModule.h"

// Hardware configuration
#define CRSF_SERIAL Serial2
#define CRSF_RX_PIN 16
#define CRSF_TX_PIN -1

// SX127x SPI Pins on ESP32 (DIO0 moved to GPIO 34 to avoid conflicts)
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  34
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23

CrsfParser parser;
ConfigManager configManager;
LoraModule lora(LORA_SS, LORA_RST, LORA_DIO0);

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
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=============================================");
    Serial.println(" ESP32 CRSF to LoRa TRANSMITTER ");
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

    // Initialize SPI and LoRa
    Serial.printf("Initializing LoRa SX127x at %u Hz...\n", activeConfig.loraFreq);
    if (!lora.begin(activeConfig.loraFreq, LORA_SCK, LORA_MISO, LORA_MOSI)) {
        Serial.println("LoRa initialization failed! Check wiring.");
    } else {
        Serial.println("LoRa initialization successful.");
    }

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

            // Broadcast via LoRa
            if (lora.sendPacket((uint8_t*)&loraPacket, sizeof(loraPacket))) {
                loraSentCount++;
            }
        }
    }

    // Yield to background tasks
    delay(1);

    // Diagnostics every 5 seconds
    if (millis() - lastReport > 5000) {
        TxConfig activeConfig = configManager.getConfig();
        Serial.printf("[TX] Bytes: %u | CRSF Packets: %u | LoRa Sent: %u | Freq: %u MHz\n",
                      byteCount, packetCount, loraSentCount, activeConfig.loraFreq / 1000000);
        lastReport = millis();
    }
}
