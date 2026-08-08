/*
 * Standalone Ebyte E32 LoRa Configuration Tool for ESP32
 *
 * This tool is used ONE-TIME to program both your Transmitter and Receiver
 * Ebyte E32 LoRa modules to have matching frequency (433MHz), baud rate (9600),
 * air speed (2.4kbps), and persistent max transmit power (30dBm / 1W).
 *
 * INSTRUCTIONS:
 * 1. Temporarily connect E32 to ESP32:
 *    - E32 TXD -> ESP32 GPIO 18 (E32_RX)
 *    - E32 RXD -> ESP32 GPIO 19 (E32_TX)
 *    - E32 M0  -> ESP32 GPIO 14 (E32_M0)
 *    - E32 M1  -> ESP32 GPIO 21 (E32_M1)
 *    - E32 VCC -> ESP32 5V (or external BEC/battery, sharing common GND)
 *    - E32 GND -> ESP32 GND
 * 2. Set "USB CDC On Boot" to "Enabled" in the Arduino IDE to view the console.
 * 3. Upload this sketch. Open Serial Monitor at 115200 baud.
 * 4. Read the success confirmation.
 * 5. After successful configuration:
 *    - Disconnect M0 and M1 from the ESP32.
 *    - PERMANENTLY CONNECT BOTH M0 AND M1 PINS OF THE E32 DIRECTLY TO GND (Mode 0: Normal Mode).
 *    - Flash your production CRSF firmware (crsf_lora_tx or crsf_lora_rx)!
 */

#include <Arduino.h>

#define E32_RX_PIN 18  // ESP32 RX (Connected to E32 TXD)
#define E32_TX_PIN 19  // ESP32 TX (Connected to E32 RXD)
#define E32_M0_PIN 14  // Connected to E32 M0
#define E32_M1_PIN 21  // Connected to E32 M1

// Target frequency configuration (default 433,000,000 Hz)
const uint32_t TARGET_FREQUENCY = 433000000;

HardwareSerial E32_Serial(2); // Use hardware Serial2

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=============================================");
    Serial.println("  EBYTE E32 LORA PERSISTENT CONFIGURATION TOOL ");
    Serial.println("=============================================");

    // Initialize Mode pins to Configuration Mode (M0=1, M1=1)
    pinMode(E32_M0_PIN, OUTPUT);
    pinMode(E32_M1_PIN, OUTPUT);
    digitalWrite(E32_M0_PIN, HIGH);
    digitalWrite(E32_M1_PIN, HIGH);

    Serial.println("[E32] Setting Mode 3 (Sleep/Configuration Mode) M0=HIGH, M1=HIGH...");
    delay(500); // Give the module plenty of time to enter Mode 3

    // Initialize UART at 9600 baud (configuration baudrate is fixed at 9600)
    E32_Serial.begin(9600, SERIAL_8N1, E32_RX_PIN, E32_TX_PIN);
    delay(100);

    // Flush any old garbage
    while (E32_Serial.available()) {
        E32_Serial.read();
    }

    // Calculate E32 Channel Byte
    uint8_t chan = 23; // Default to 433MHz (channel 23: 410 + 23 = 433MHz)
    if (TARGET_FREQUENCY >= 862000000 && TARGET_FREQUENCY <= 893000000) {
        chan = (TARGET_FREQUENCY - 862000000) / 1000000;
    } else if (TARGET_FREQUENCY >= 900000000 && TARGET_FREQUENCY <= 931000000) {
        chan = (TARGET_FREQUENCY - 900000000) / 1000000;
    } else if (TARGET_FREQUENCY >= 410000000 && TARGET_FREQUENCY <= 441000000) {
        chan = (TARGET_FREQUENCY - 410000000) / 1000000;
    }

    // 6-byte Ebyte E32 Configuration Command
    // Byte 0: 0xC0 (Write parameters persistently to non-volatile EEPROM)
    // Byte 1: ADDH = 0x00 (Default Address High)
    // Byte 2: ADDL = 0x00 (Default Address Low)
    // Byte 3: SPED = 0x1A (8N1 Parity, 9600 baud, 2.4kbps air data rate for max sensitivity/range)
    // Byte 4: CHAN = calculated chan (Frequency mapping)
    // Byte 5: OPTION = 0x44 (Transparent transceive, Max 30dBm/1W power boost enabled)
    uint8_t cmd[6] = { 0xC0, 0x00, 0x00, 0x1A, chan, 0x44 };

    Serial.print("[E32] Sending Persistent Configuration parameters: ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("0x%02X ", cmd[i]);
    }
    Serial.println();

    E32_Serial.write(cmd, 6);
    E32_Serial.flush();
    delay(200);

    // Read response from E32 (it echoes the configured parameters starting with 0xC0 or 0xC2)
    Serial.print("[E32] Response from module: ");
    uint32_t startMs = millis();
    int readBytes = 0;
    uint8_t response[6] = {0};

    while (millis() - startMs < 2000 && readBytes < 6) {
        if (E32_Serial.available()) {
            response[readBytes] = E32_Serial.read();
            Serial.printf("0x%02X ", response[readBytes]);
            readBytes++;
        }
    }
    Serial.println();

    if (readBytes == 6 && (response[0] == 0xC0 || response[0] == 0xC2)) {
        Serial.println("\nSUCCESS: Ebyte E32 module has been persistently programmed!");
        Serial.printf("- Address: 0x%02X%02X\n", response[1], response[2]);
        Serial.printf("- Speed Byte: 0x%02X (2.4kbps air rate, 9600 baud)\n", response[3]);
        Serial.printf("- Channel Byte: 0x%02X (%u MHz)\n", response[4], 410 + response[4]);
        Serial.printf("- Option Byte: 0x%02X (Max Power 1W/30dBm, Transparent Mode)\n", response[5]);
        Serial.println("\nNEXT STEPS:");
        Serial.println("1. Unplug the ESP32 from USB.");
        Serial.println("2. Disconnect E32 M0 and M1 pins from the ESP32.");
        Serial.println("3. PERMANENTLY CONNECT BOTH M0 AND M1 PINS DIRECTLY TO GND (Mode 0: Normal Mode).");
        Serial.println("4. Upload your production mesh firmwares (crsf_lora_tx or crsf_lora_rx)!");
    } else {
        Serial.println("\nERROR: No response or invalid response from Ebyte E32 module.");
        Serial.println("Please double-check your TXD/RXD/M0/M1 wiring and power connections.");
    }
}

void loop() {
    // Idle
}
