/*
 * CRSF LoRa Receiver & Outputs Controller for ESP32 (Dev Module)
 * - Receives channel broadcast from Transmitter via SX127x LoRa Module.
 * - Controls 2 synchronous servos (Left on GPIO 4, Right on GPIO 13) with optional Right Servo Inversion.
 * - Controls MOSFET on GPIO 25 and Extra Pin on GPIO 26.
 * - Monitor 2 Limit Switches: Upper (GPIO 32) and Lower (GPIO 33) using INPUT_PULLUP.
 * - If at least one limit switch is open (reads HIGH), MOSFET and Extra Pin are forced to configured Off Level (HIGH/LOW).
 * - LED (GPIO 27) lights up when Upper Limit Switch is open.
 * - Hosts a local Web Configurator Access Point (AP SSID: "CRSF-Config-RX") with WiFi TX power reduced to 25%.
 * - Supports "All one channel" configuration option to duplicate settings.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "ConfigManager.h"
#include "PwmController.h"
#include "WebServerHandler.h"
#include "LoraModule.h"

// Pin Definitions for ESP32 Dev Module
// Note: Changed LEFT_SERVO_PIN from 12 to 4 because GPIO 12 (MTDI) is an ESP32 strapping pin.
// If GPIO 12 is held HIGH at boot (by servo pull-ups), SPI Flash voltage falls to 1.8V,
// causing 'Failed to communicate with the flash chip' uploading error. GPIO 4 is fully safe.
#define LEFT_SERVO_PIN  4
#define RIGHT_SERVO_PIN 13
#define MOSFET_PIN      25
#define EXTRA_PIN       26
#define LED_PIN         27
#define UPPER_SW_PIN    32
#define LOWER_SW_PIN    33

// LoRa SPI Pin Configuration
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  2
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23

ConfigManager configManager;
PwmController controller(LEFT_SERVO_PIN, RIGHT_SERVO_PIN, MOSFET_PIN, EXTRA_PIN, LED_PIN, UPPER_SW_PIN, LOWER_SW_PIN);
LoraModule lora(LORA_SS, LORA_RST, LORA_DIO0);

// Global shared variables
uint32_t packetCount = 0;
uint16_t channels[16];
bool upperSwOpen = false;
bool lowerSwOpen = false;
bool overrideActive = false;

// Debounce variables for mechanical limit switches to prevent high-frequency EMI resets
uint32_t lastUpperSwTime = 0;
uint32_t lastLowerSwTime = 0;
bool debouncedUpperSw = false;
bool debouncedLowerSw = false;
const uint32_t DEBOUNCE_DELAY_MS = 50; // 50ms stable window

struct LoraPacket {
    uint16_t signature; // 0x55AA
    uint32_t packetId;
    uint16_t channels[16];
} loraPacket;

WebServerHandler webServer(configManager, packetCount, channels, upperSwOpen, lowerSwOpen, overrideActive);

uint32_t lastReport = 0;

// Helper to check if channel triggers on a specific position with tolerances
bool isTriggerActive(uint16_t val, uint16_t triggerPosition) {
    if (triggerPosition == 1500) {
        return (val >= 1400 && val <= 1600);
    } else {
        // Tolerances of ±50 around edge targets (e.g. 1000 -> 950-1050, 2000 -> 1950-2050)
        return (val >= (triggerPosition - 50) && val <= (triggerPosition + 50));
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=============================================");
    Serial.println(" ESP32 CRSF LoRa RX OUTPUT CONTROLLER ");
    Serial.println("=============================================");

    // Initialize Channels array with neutral value (1500us/ticks)
    for (int i = 0; i < 16; i++) {
        channels[i] = 1500;
    }

    // Load configurations
    configManager.begin();
    RxConfig activeConfig = configManager.getConfig();

    // Start Web Server
    webServer.begin();

    // Initialize LoRa SPI and Receiver
    Serial.printf("Initializing LoRa SX127x at %u Hz...\n", activeConfig.loraFreq);
    if (!lora.begin(activeConfig.loraFreq, LORA_SCK, LORA_MISO, LORA_MOSI)) {
        Serial.println("LoRa initialization failed! Check wiring.");
    } else {
        Serial.println("LoRa initialization successful. Starting continuous receive mode.");
        lora.startReceive();
    }

    // Initialize Outputs and run self-test sweep
    controller.begin();
}

void loop() {
    // 1. Handle Web requests
    webServer.handleClient();

    // 2. Poll LoRa for incoming packets
    int packetSize = lora.parsePacket();
    if (packetSize >= (int)sizeof(LoraPacket)) {
        LoraPacket tempPacket;
        int len = lora.readPacket((uint8_t*)&tempPacket, sizeof(tempPacket));
        if (len == sizeof(LoraPacket) && tempPacket.signature == 0x55AA) {
            packetCount++;
            for (int i = 0; i < 16; i++) {
                channels[i] = tempPacket.channels[i];
            }
        }
        // Return to receiving mode
        lora.startReceive();
    }

    // 3. Read Limit Switches & Apply Software Debounce (prevents high-frequency switching and power reboots)
    bool rawUpper = (digitalRead(UPPER_SW_PIN) == HIGH);
    bool rawLower = (digitalRead(LOWER_SW_PIN) == HIGH);

    if (rawUpper != debouncedUpperSw) {
        if (millis() - lastUpperSwTime > DEBOUNCE_DELAY_MS) {
            debouncedUpperSw = rawUpper;
            lastUpperSwTime = millis();
        }
    } else {
        lastUpperSwTime = millis();
    }

    if (rawLower != debouncedLowerSw) {
        if (millis() - lastLowerSwTime > DEBOUNCE_DELAY_MS) {
            debouncedLowerSw = rawLower;
            lastLowerSwTime = millis();
        }
    } else {
        lastLowerSwTime = millis();
    }

    upperSwOpen = debouncedUpperSw;
    lowerSwOpen = debouncedLowerSw;

    // LED glows if Upper switch is open
    digitalWrite(LED_PIN, upperSwOpen ? HIGH : LOW);

    // 4. Run Trigger Logic and update Servos/MOSFET
    RxConfig activeConfig = configManager.getConfig();

    bool servoActive = isTriggerActive(channels[activeConfig.servoChannel - 1], activeConfig.servoTrigger);
    bool mosfetActive = isTriggerActive(channels[activeConfig.mosfetChannel - 1], activeConfig.mosfetTrigger);

    // Limit switch safety override
    if (upperSwOpen || lowerSwOpen) {
        overrideActive = true;
        // Force MOSFET & Extra Pin to the safety Off Level
        controller.updateOutputs(false, activeConfig.mosfetOffLevel);
    } else {
        overrideActive = false;
        // Run normally
        controller.updateOutputs(mosfetActive, activeConfig.mosfetOffLevel);
    }

    // Update Servos
    controller.updateServos(servoActive, activeConfig.servoMin, activeConfig.servoMax, activeConfig.servoInvertRight != 0);

    // Yield to WiFi/TCP tasks
    delay(1);

    // 5. Diagnostics reporting
    if (millis() - lastReport > 5000) {
        Serial.printf("[RX] Received LoRa Packets: %u | UpperSw: %s | LowerSw: %s | Override: %s | ServoPairActive: %s | MosfetActive: %s\n",
                      packetCount,
                      upperSwOpen ? "OPEN (Triggered)" : "CLOSED (OK)",
                      lowerSwOpen ? "OPEN (Triggered)" : "CLOSED (OK)",
                      overrideActive ? "ACTIVE" : "NONE",
                      servoActive ? "YES" : "NO",
                      mosfetActive ? "YES" : "NO");
        lastReport = millis();
    }
}
