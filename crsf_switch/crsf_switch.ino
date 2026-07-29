/*
 * CRSF to PWM Switch, Servo, & Camera Controller for ESP32-C3 Super Mini
 * - Listens to CRSF stream on Pins 6 & 7.
 * - Supports three dynamic PWM outputs: Switch (Pin 5), Servo (Pin 4), and Camera Switch (Pin 3).
 * - Custom Minimum/Maximum pulse widths configured dynamically via Local Web UI.
 * - AP Config Web Portal ("CRSF-Config" at http://192.168.4.1) allows custom output mapping.
 * - Supports live Web Status Monitor displaying raw serial byte reception and channel values.
 * - Supports selecting dynamic baudrates (115200, 400000, 420000).
 */

#include "CrsfParser.h"
#include "PwmController.h"
#include "ConfigManager.h"
#include "WebServerHandler.h"

// Hardware Pin Configuration
#define CRSF_SERIAL Serial1
#define RX_PIN 6
#define TX_PIN -1 // Receive-only mode: ESP32-C3 only listens to CRSF TX line, does not transmit back
#define SWITCH_PIN 5
#define SERVO_PIN 4
#define CAMERA_PIN 3

CrsfParser parser;
ConfigManager configManager;
PwmController controller(SWITCH_PIN, SERVO_PIN, CAMERA_PIN);

// Shared diagnostic counters
uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t lastReport = 0;
uint16_t lastSwitchVal = 0;
uint16_t lastServoVal = 0;
uint16_t lastCameraVal = 0;

WebServerHandler webServer(configManager, parser, byteCount, packetCount);

void setup() {
  // Built-in USB CDC Serial
  Serial.begin(115200);
  delay(3000); // Wait for Serial Monitor

  Serial.println("\n=============================================");
  Serial.println(" ESP32-C3 CRSF TRIPLE OUTPUT CONTROLLER ");
  Serial.println("=============================================");
  Serial.printf("Configured Pins: RX=%d, TX=%d, Switch=%d, Servo=%d, Camera=%d\n",
                RX_PIN, TX_PIN, SWITCH_PIN, SERVO_PIN, CAMERA_PIN);

  // Load Saved Configuration
  configManager.begin();
  OutputConfig activeConfig = configManager.getConfig();

  // Initialize WiFi AP & Web Server
  webServer.begin();

  // Initialize Hardware UART for CRSF with Dynamic Baudrate
  Serial.printf("Initializing CRSF on Serial1 at %u baud...\n", activeConfig.crsfBaudrate);
  CRSF_SERIAL.begin(activeConfig.crsfBaudrate, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initialize PWM Controllers with Diagnostic Boot Sweeps
  controller.begin();
}

void loop() {
  // 1. Process client configuration requests & AJAX status requests
  webServer.handleClient();

  // 2. Parse CRSF stream (with a safety limit to prevent trapping CPU and starving WiFi task)
  uint8_t readLimit = 0;
  while (CRSF_SERIAL.available() && readLimit < 128) {
    uint8_t b = CRSF_SERIAL.read();
    byteCount++;
    readLimit++;

    if (parser.processByte(b)) {
      packetCount++;

      OutputConfig activeConfig = configManager.getConfig();

      uint16_t swVal = parser.getChannel(activeConfig.switchChannel - 1);
      uint16_t srvVal = parser.getChannel(activeConfig.servoChannel - 1);
      uint16_t camVal = parser.getChannel(activeConfig.cameraChannel - 1);

      // Update hardware outputs with custom min/max PWM ranges (microseconds)
      controller.updateSwitch(swVal, activeConfig.switchMin, activeConfig.switchMax);
      controller.updateServo(srvVal, activeConfig.servoMin, activeConfig.servoMax);
      controller.updateCamera(camVal, activeConfig.cameraMin, activeConfig.cameraMax);

      // Hardware Serial monitor outputs on value change
      if (abs((int)swVal - (int)lastSwitchVal) > 10 ||
          abs((int)srvVal - (int)lastServoVal) > 10 ||
          abs((int)camVal - (int)lastCameraVal) > 10) {

        Serial.printf("Live CRSF Update -> Switch(Ch%d): %u, Servo(Ch%d): %u, Camera(Ch%d): %u\n",
                      activeConfig.switchChannel, swVal,
                      activeConfig.servoChannel, srvVal,
                      activeConfig.cameraChannel, camVal);

        lastSwitchVal = swVal;
        lastServoVal = srvVal;
        lastCameraVal = camVal;
      }
    }
  }

  // Feed/Yield to system tasks (TCP/IP and WiFi stack) to maximize network/CPU stability
  delay(1);

  // 3. Hardware Serial diagnostic reports every 5 seconds
  if (millis() - lastReport > 5000) {
    if (byteCount == 0) {
      Serial.println("HARDWARE SERIAL MONITOR: No data received. Check RX wiring.");
    } else {
      OutputConfig activeConfig = configManager.getConfig();
      Serial.printf("HARDWARE SERIAL MONITOR: Bytes: %u | RC Packets: %u | Sw(Ch%d): %u | Srv(Ch%d): %u | Cam(Ch%d): %u\n",
                    byteCount, packetCount,
                    activeConfig.switchChannel, parser.getChannel(activeConfig.switchChannel - 1),
                    activeConfig.servoChannel, parser.getChannel(activeConfig.servoChannel - 1),
                    activeConfig.cameraChannel, parser.getChannel(activeConfig.cameraChannel - 1));
    }
    lastReport = millis();
  }
}
