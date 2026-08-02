/*
 * CRSF LoRa Receiver & Outputs Controller for ESP32 (Dev Module)
 * - Receives channel broadcast from Transmitter via SX127x LoRa Module.
 * - Controls 2 synchronous servos (Left on GPIO 4, Right on GPIO 13) with Left and Right software inversions.
 * - Monitors 2 Limit Switches:
 *   1. Upper (GPIO 32) using INPUT_PULLUP.
 *   2. Lower (GPIO 33) using INPUT_PULLUP.
 * - LED (GPIO 27) lights up when Upper Limit Switch is open (HIGH).
 * - Automatic WiFi and AP shutdown 3 minutes after boot (180,000 ms) to reduce noise.
 * - Implement 8-box mesh selection logic using Box Selection Channel (1000-2000us range partitioned in 8 segments).
 * - If Box matches this unit's Box ID:
 *   - Servo Trigger channel is evaluated to drive Servos (UP to 2000us, Neutral/Down to 1500us).
 *   - When active, MOSFET (GPIO 26) outputs 1500us 50Hz PWM, and Extra Pin (GPIO 15) outputs 2000us 50Hz PWM.
 * - If any limit switch is open (HIGH):
 *   - MOSFET and Extra Pin are forced to the configured OFF level (1000us or 2000us PWM).
 * - Reduced WiFi Transmit Power to 25% (WIFI_POWER_5dBm).
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
#define MOSFET_PIN      26 // External MOSFET (GPIO 26)
#define EXTRA_PIN       15 // Separate mirror pin (GPIO 15)
#define LED_PIN         27 // Status Indicator LED
#define UPPER_SW_PIN    32 // Upper Limit Switch
#define LOWER_SW_PIN    33 // Lower Limit Switch

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
bool upperSwOpen = false;       // True if Upper Switch is open (HIGH)
bool lowerSwOpen = false;       // True if Lower Switch is open (HIGH)
bool overrideActive = false;    // True if at least one switch is open

// Debounce variables for mechanical limit switches
uint32_t lastUpperSwTime = 0;
uint32_t lastLowerSwTime = 0;

bool debouncedUpperSw = false; // true = pin is HIGH, false = pin is LOW
bool debouncedLowerSw = false; // true = pin is HIGH, false = pin is LOW

const uint32_t DEBOUNCE_DELAY_MS = 50; // 50ms stable window

struct LoraPacket {
    uint16_t signature; // 0x55AA
    uint32_t packetId;
    uint16_t channels[16];
} loraPacket;

WebServerHandler webServer(configManager, packetCount, channels, upperSwOpen, lowerSwOpen, overrideActive);

uint32_t lastReport = 0;
bool wifiShutDownDone = false;

// Helper to check if channel triggers on a specific position with tolerances
bool isTriggerActive(uint16_t val, uint16_t triggerPosition) {
    if (triggerPosition == 1500) {
        return (val >= 1400 && val <= 1600);
    } else {
        // Tolerances of ±50 around edge targets (e.g. 1000 -> 950-1050, 2000 -> 1950-2050)
        return (val >= (triggerPosition - 50) && val <= (triggerPosition + 50));
    }
}

// Partition 1000-2000 us range into 8 equal segments (125us per segment)
int getSelectedBox(uint16_t pulseWidth) {
    if (pulseWidth < 1000) pulseWidth = 1000;
    if (pulseWidth > 2000) pulseWidth = 2000;
    int box = ((pulseWidth - 1000) / 125) + 1;
    if (box < 1) box = 1;
    if (box > 8) box = 8;
    return box;
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=============================================");
    Serial.println(" ESP32 CRSF LoRa RX OUTPUT CONTROLLER ");
    Serial.println("=============================================");

    // Initialize Channels array with neutral/mid value
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

    // Evaluate Upper Limit Switch state at boot to initialize position organically
    bool isUpperTriggeredAtBoot = (digitalRead(UPPER_SW_PIN) == HIGH);

    // Initialize Outputs (Servos)
    controller.begin(activeConfig.servoInvertLeft != 0, activeConfig.servoInvertRight != 0, activeConfig.servoMin, activeConfig.servoMax, isUpperTriggeredAtBoot);
}

void loop() {
    // 1. Automatic WiFi shutdown after 3 minutes (180,000 ms)
    if (!wifiShutDownDone && millis() > 180000) {
        Serial.println("[System] 3 minutes elapsed. Disabling WiFi and SoftAP to conserve power and reduce RF noise.");
        WiFi.mode(WIFI_OFF);
        wifiShutDownDone = true;
    }

    // 2. Handle Web requests (if WiFi is still on)
    if (!wifiShutDownDone) {
        webServer.handleClient();
    }

    // 3. Poll LoRa for incoming packets
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
        lora.startReceive();
    }

    // 4. Read Limit Switches & Apply Software Debounce
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

    // 5. Run Trigger Logic and update Outputs
    RxConfig activeConfig = configManager.getConfig();

    // Switch is open if debounced input reads HIGH
    upperSwOpen = debouncedUpperSw;
    lowerSwOpen = debouncedLowerSw;

    // LED glows if Upper switch is open
    digitalWrite(LED_PIN, upperSwOpen ? HIGH : LOW);

    // Determine current selected Box ID from the Selector Channel
    uint16_t boxSelVal = channels[activeConfig.boxSelectChannel - 1];
    int currentSelectedBox = getSelectedBox(boxSelVal);

    bool isMyBoxSelected = (currentSelectedBox == activeConfig.boxId);

    // Servo trigger evaluation
    bool triggerActive = isTriggerActive(channels[activeConfig.servoChannel - 1], activeConfig.servoTrigger);

    bool servoActive = false;
    bool mosfetActive = false;
    bool extraActive = false;

    // Safety Override: if either limit switch is open, force disabled states
    bool isAnyOpen = upperSwOpen || lowerSwOpen;
    overrideActive = isAnyOpen;

    if (isMyBoxSelected) {
        if (triggerActive) {
            servoActive = true;             // Move servos to 2000us
            if (!isAnyOpen) {
                mosfetActive = true;        // Output 1500us PWM on MOSFET (GPIO 26)
                extraActive = true;         // Output 2000us PWM on Extra (GPIO 15)
            }
        } else {
            servoActive = false;            // Return servos to 1500us (neutral)
            mosfetActive = false;
            extraActive = false;
        }
    } else {
        // Not selected
        servoActive = false;
        mosfetActive = false;
        extraActive = false;
    }

    // Update physical servos with 250us per 60 seconds slow rate limit
    controller.updateServos(servoActive, activeConfig.servoMin, activeConfig.servoMax, activeConfig.servoInvertLeft != 0, activeConfig.servoInvertRight != 0);

    // Update 50Hz LEDC PWM Outputs: GPIO 26 (MOSFET) and GPIO 15 (Extra)
    controller.updatePwmOutputs(mosfetActive, extraActive, activeConfig.mosfetOffLevel);

    yield();

    // 6. Diagnostics reporting
    if (millis() - lastReport > 5000) {
        Serial.printf("[RX] SelectedBox:%d (MyBox:%d, Selected:%s) | UpperOpen: %s | LowerOpen: %s | Servos: %s | MOSFET: %s | Extra: %s\n",
                      currentSelectedBox,
                      activeConfig.boxId,
                      isMyBoxSelected ? "YES" : "NO",
                      upperSwOpen ? "YES" : "NO",
                      lowerSwOpen ? "YES" : "NO",
                      servoActive ? "ACTIVE (2000us)" : "NEUTRAL (1500us)",
                      mosfetActive ? "ACTIVE (1500us)" : "OFF",
                      extraActive ? "ACTIVE (2000us)" : "OFF");
        lastReport = millis();
    }
}
