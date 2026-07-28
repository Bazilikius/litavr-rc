/*
 * CRSF to PWM Switch & Servo Controller for ESP32-C3 Super Mini
 * - Listens to CRSF stream (400k baud) on Pins 6 & 7.
 * - Extracts two user-defined CRSF Channels to control Switch (Pin 5) & Servo (Pin 4).
 * - Serves a local WiFi Web Server (AP: "CRSF-Config") to customize channel mapping.
 */

#include "CrsfParser.h"
#include "PwmController.h"
#include "ConfigManager.h"
#include "WebServerHandler.h"

// Hardware Pin Configuration
#define CRSF_SERIAL Serial1
#define CRSF_BAUDRATE 400000
#define RX_PIN 6
#define TX_PIN 7
#define SWITCH_PIN 5
#define SERVO_PIN 4
#define LEDC_CHANNEL_0 0
#define LEDC_CHANNEL_1 1

CrsfParser parser;
ConfigManager configManager;
PwmController controller(SWITCH_PIN, SERVO_PIN);
WebServerHandler webServer(configManager);

void setup() {
  // Debugging output on built-in USB CDC Serial
  Serial.begin(115200);
  delay(3000); // Wait for Serial monitor connection

  Serial.println("\n=============================================");
  Serial.println(" ESP32-C3 CRSF DUAL OUTPUT CONTROLLER ");
  Serial.println("=============================================");
  Serial.printf("Pins: RX=%d, TX=%d, Switch=%d, Servo=%d\n", RX_PIN, TX_PIN, SWITCH_PIN, SERVO_PIN);

  // Initialize Configurations
  configManager.begin();

  // Initialize Web Configuration AP Portal
  webServer.begin();

  // Initialize Hardware UART for CRSF
  CRSF_SERIAL.begin(CRSF_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initialize PWM Controllers
  controller.begin();
}

uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t lastReport = 0;
uint16_t lastSwitchVal = 0;
uint16_t lastServoVal = 0;

void loop() {
  // 1. Handle incoming Web Client configurations
  webServer.handleClient();

  // 2. Parse incoming CRSF receiver data
  while (CRSF_SERIAL.available()) {
    uint8_t b = CRSF_SERIAL.read();
    byteCount++;

    if (parser.processByte(b)) {
      packetCount++;

      // Get current channel mappings from non-volatile storage
      OutputConfig activeConfig = configManager.getConfig();

      uint16_t swVal = parser.getChannel(activeConfig.switchChannel - 1);
      uint16_t srvVal = parser.getChannel(activeConfig.servoChannel - 1);

      // Update physical PWM outputs
      controller.updateSwitch(swVal);
      controller.updateServo(srvVal);

      // Log any notable value changes
      if (abs((int)swVal - (int)lastSwitchVal) > 10 || abs((int)srvVal - (int)lastServoVal) > 10) {
        Serial.printf("Updates -> Switch(Ch%d): %u, Servo(Ch%d): %u\n",
                      activeConfig.switchChannel, swVal,
                      activeConfig.servoChannel, srvVal);
        lastSwitchVal = swVal;
        lastServoVal = srvVal;
      }
    }
  }

  // 3. Periodic diagnostic reports
  if (millis() - lastReport > 5000) {
    if (byteCount == 0) {
      Serial.println("DIAGNOSTIC: No CRSF data received. Check hardware RX line.");
    } else {
      OutputConfig activeConfig = configManager.getConfig();
      Serial.printf("DIAGNOSTIC: Bytes=%u, Packets=%u, Switch=Ch%d (%u), Servo=Ch%d (%u)\n",
                    byteCount, packetCount,
                    activeConfig.switchChannel, parser.getChannel(activeConfig.switchChannel - 1),
                    activeConfig.servoChannel, parser.getChannel(activeConfig.servoChannel - 1));
    }
    lastReport = millis();
  }
}
