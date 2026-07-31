/*
 * CRSF LoRa Receiver & Outputs Controller for ESP32 (Dev Module)
 * - Receives channel broadcast from Transmitter via SX127x LoRa Module.
 * - Controls 2 synchronous servos (Left on GPIO 4, Right on GPIO 13) with Left and Right software inversions.
 * - Controls Extra Pin on GPIO 26 (replicates the active/trigger level of the servos).
 * - Monitor 3 Limit Switches:
 *   1. Upper (GPIO 32) using INPUT_PULLUP.
 *   2. Lower (GPIO 33) using INPUT_PULLUP.
 *   3. Servo UP Switch (GPIO 25) using INPUT_PULLUP.
 * - LED (GPIO 27) lights up when Upper Limit Switch is open (reads HIGH).
 * - If Servo UP Switch (GPIO 25) is pressed (reads LOW), servos are overridden and driven UP (active position).
 * - Hosts a local Web Configurator Access Point (AP SSID: "CRSF-Config-RX") with WiFi TX power reduced to 25%.
 */

#include <Arduino.h>
#include <WiFi.h>
#include "ConfigManager.h"
#include "PwmController.h"
#include "WebServerHandler.h"
#include "LoraModule.h"

// Pin Definitions for ESP32 Dev Module
#define LEFT_SERVO_PIN  4
#define RIGHT_SERVO_PIN 13
#define MOSFET_PIN      25 // Re-purposed as a limit switch input to move servos UP
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

    // Initialize Outputs with correct left/right inversions and run synchronized self-test sweep
    controller.begin(activeConfig.servoInvertLeft != 0, activeConfig.servoInvertRight != 0, activeConfig.servoMin, activeConfig.servoMax);
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

    // 4. Run Trigger Logic and update Servos/Extra Pin
    RxConfig activeConfig = configManager.getConfig();

    bool servoActive = isTriggerActive(channels[activeConfig.servoChannel - 1], activeConfig.servoTrigger);

    // Override servos to move UP if the physical limit switch on GPIO 25 is pressed (reads LOW)
    bool servoUpSwPressed = controller.isMosfetSwPressed();
    if (servoUpSwPressed) {
        servoActive = true;
    }

    // Replicate active servo state to Extra Pin (GPIO 26): HIGH when active (servos UP), LOW when inactive (servos DOWN)
    digitalWrite(EXTRA_PIN, servoActive ? HIGH : LOW);

    // Update Servos based on active state
    controller.updateServos(servoActive, activeConfig.servoMin, activeConfig.servoMax, activeConfig.servoInvertLeft != 0, activeConfig.servoInvertRight != 0);

    // Yield to WiFi/TCP tasks
    delay(1);

    // 5. Diagnostics reporting
    if (millis() - lastReport > 5000) {
        Serial.printf("[RX] Received LoRa: %u | UpperSw: %s | LowerSw: %s | ServoUpSw: %s | ServosActive (UP): %s\n",
                      packetCount,
                      upperSwOpen ? "OPEN (Triggered)" : "CLOSED (OK)",
                      lowerSwOpen ? "OPEN (Triggered)" : "CLOSED (OK)",
                      servoUpSwPressed ? "PRESSED" : "RELEASED",
                      servoActive ? "YES" : "NO");
        lastReport = millis();
    }
}
