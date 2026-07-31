/*
 * CRSF LoRa Receiver & Outputs Controller for ESP32 (Dev Module)
 * - Receives channel broadcast from Transmitter via SX127x LoRa Module.
 * - Controls 2 synchronous servos (Left on GPIO 4, Right on GPIO 13) with Left and Right software inversions.
 * - Controls Extra Pin on GPIO 26 (replicates the active/trigger level of the servos).
 * - Controls Power Key Pin on GPIO 15 (outputs 0V/5V) with a 60-second delay logic.
 * - Monitors 3 Limit Switches:
 *   1. Upper (GPIO 32) using INPUT_PULLUP.
 *   2. Lower (GPIO 33) using INPUT_PULLUP.
 *   3. Servo UP Switch (GPIO 25) using INPUT_PULLUP.
 * - LED (GPIO 27) lights up when Upper Limit Switch is open/triggered.
 * - All three switches have configurable active trigger polarities (Normally Open / Normally Closed).
 * - If Servo UP Switch (GPIO 25) state matches its configured trigger polarity, servos are driven UP (active).
 * - Power Key Pin (GPIO 15) is set HIGH only if:
 *   a) 60 seconds have elapsed since the CRSF channel commanded the servos to go DOWN.
 *   b) AND simultaneously: ALL THREE limit switches are in their non-triggered (Closed / Safe / OK) states.
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
#define POWER_KEY_PIN   15 // Pin to control the 0V/+5V key

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
bool upperSwOpen = false;       // Used to indicate Upper Switch active trigger state
bool lowerSwOpen = false;       // Used to indicate Lower Switch active trigger state
bool servoUpSwTriggered = false; // Used to indicate Servo UP Switch active trigger state
bool overrideActive = false;    // Re-purposed to convey if the system is NOT fully closed/safe

// Debounce variables for mechanical limit switches to prevent high-frequency EMI resets
uint32_t lastUpperSwTime = 0;
uint32_t lastLowerSwTime = 0;
uint32_t lastMosfetSwTime = 0;

bool debouncedUpperSw = false; // true = pin is HIGH, false = pin is LOW
bool debouncedLowerSw = false; // true = pin is HIGH, false = pin is LOW
bool debouncedMosfetSw = false; // true = pin is HIGH, false = pin is LOW

const uint32_t DEBOUNCE_DELAY_MS = 50; // 50ms stable window

// Timer for the 60-second power key delay
uint32_t loweredTimestamp = 0;

struct LoraPacket {
    uint16_t signature; // 0x55AA
    uint32_t packetId;
    uint16_t channels[16];
} loraPacket;

WebServerHandler webServer(configManager, packetCount, channels, upperSwOpen, lowerSwOpen, servoUpSwTriggered, overrideActive);

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

    // Initialize Power Key Pin
    pinMode(POWER_KEY_PIN, OUTPUT);
    digitalWrite(POWER_KEY_PIN, LOW);
    Serial.printf("Power Key initialized on GPIO %d (set to LOW).\n", POWER_KEY_PIN);

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
    bool rawMosfet = (digitalRead(MOSFET_PIN) == HIGH);

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

    if (rawMosfet != debouncedMosfetSw) {
        if (millis() - lastMosfetSwTime > DEBOUNCE_DELAY_MS) {
            debouncedMosfetSw = rawMosfet;
            lastMosfetSwTime = millis();
        }
    } else {
        lastMosfetSwTime = millis();
    }

    // 4. Run Trigger Logic and update Servos/Extra Pin
    RxConfig activeConfig = configManager.getConfig();

    // Evaluate switch states based on their configured polarities (0 = Active LOW/Closed, 1 = Active HIGH/Open)
    upperSwOpen = (debouncedUpperSw == (activeConfig.upperSwPolarity != 0));
    lowerSwOpen = (debouncedLowerSw == (activeConfig.lowerSwPolarity != 0));
    servoUpSwTriggered = (debouncedMosfetSw == (activeConfig.servoUpSwPolarity != 0));

    // LED glows if Upper switch is triggered
    digitalWrite(LED_PIN, upperSwOpen ? HIGH : LOW);

    // Check CRSF channel state independently for the 60-second power-down delay
    bool crsfServoActive = isTriggerActive(channels[activeConfig.servoChannel - 1], activeConfig.servoTrigger);
    if (!crsfServoActive) {
        if (loweredTimestamp == 0) {
            loweredTimestamp = millis();
        }
    } else {
        loweredTimestamp = 0; // Reset
    }

    bool servoActive = crsfServoActive;

    // Override servos to move UP if the physical limit switch on GPIO 25 is in its triggered state
    if (servoUpSwTriggered) {
        servoActive = true;
    }

    // Replicate active servo state to Extra Pin (GPIO 26): HIGH when active (servos UP), LOW when inactive (servos DOWN)
    digitalWrite(EXTRA_PIN, servoActive ? HIGH : LOW);

    // 60-second delay logic for Power Key (GPIO 15):
    // Activates (HIGH) 60 seconds after the CRSF channel commands servos to go DOWN,
    // AND simultaneously: ALL THREE limit switches are in their non-triggered / safe / closed (OK) state.
    bool elapsed60s = (loweredTimestamp != 0 && (millis() - loweredTimestamp >= 60000));
    bool allThreeClosed = (!upperSwOpen) && (!lowerSwOpen) && (!servoUpSwTriggered);

    overrideActive = !allThreeClosed; // Show if they are not all closed

    bool powerKeyOn = elapsed60s && allThreeClosed;
    digitalWrite(POWER_KEY_PIN, powerKeyOn ? HIGH : LOW);

    // Update Servos based on active state
    controller.updateServos(servoActive, activeConfig.servoMin, activeConfig.servoMax, activeConfig.servoInvertLeft != 0, activeConfig.servoInvertRight != 0);

    // Yield to WiFi/TCP tasks
    delay(1);

    // 5. Diagnostics reporting
    if (millis() - lastReport > 5000) {
        uint32_t secondsDown = (loweredTimestamp != 0) ? (millis() - loweredTimestamp) / 1000 : 0;
        Serial.printf("[RX] Received LoRa: %u | UpperSwTriggered: %s | LowerSwTriggered: %s | ServoUpSwTriggered: %s | ServosActive (UP): %s | PowerKey (GPIO 15): %s (Down for %u s)\n",
                      packetCount,
                      upperSwOpen ? "YES" : "NO",
                      lowerSwOpen ? "YES" : "NO",
                      servoUpSwTriggered ? "YES" : "NO",
                      servoActive ? "YES" : "NO",
                      powerKeyOn ? "ON (5V)" : "OFF (0V)",
                      secondsDown);
        lastReport = millis();
    }
}
