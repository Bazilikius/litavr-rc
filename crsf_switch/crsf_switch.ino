/*
 * CRSF to PWM Switch Controller for ESP32-C3 Super Mini
 * Listens to Channel 15 from CRSF stream and controls a PWM output.
 */

#include "CrsfParser.h"
#include "PwmController.h"

// Configuration
#define CRSF_SERIAL Serial1
#define CRSF_BAUDRATE 400000
#define RX_PIN 6
#define TX_PIN 7
#define PWM_PIN 5
#define LEDC_CHANNEL 0
#define TARGET_CHANNEL 15

CrsfParser parser;
PwmController controller(PWM_PIN, LEDC_CHANNEL);

void setup() {
  // Built-in USB Serial for debugging
  Serial.begin(115200);
  delay(3000); // Wait for Serial monitor to connect

  Serial.println("\n=====================================");
  Serial.println(" ESP32-C3 CRSF TO PWM CONTROLLER ");
  Serial.println("=====================================");
  Serial.printf("Config: RX=%d, TX=%d, PWM=%d, Baud=%d\n", RX_PIN, TX_PIN, PWM_PIN, CRSF_BAUDRATE);
  Serial.println("IMPORTANT: Ensure a COMMON GROUND (GND) between ESP32 and switch.");

  // Initialize Hardware UART for CRSF
  CRSF_SERIAL.begin(CRSF_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initialize PWM Controller
  controller.begin();
}

uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t lastReport = 0;
uint16_t lastCh15 = 0;

void loop() {
  while (CRSF_SERIAL.available()) {
    uint8_t b = CRSF_SERIAL.read();
    byteCount++;

    if (parser.processByte(b)) {
      packetCount++;
      uint16_t ch15 = parser.getChannel(TARGET_CHANNEL - 1);

      // Update PWM
      controller.update(ch15);

      // Log on value change
      if (abs((int)ch15 - (int)lastCh15) > 5) {
        Serial.printf("Value Change - Ch15: %u -> Updating PWM\n", ch15);
        lastCh15 = ch15;
      }
    }
  }

  // Periodic diagnostic report every 5 seconds
  if (millis() - lastReport > 5000) {
    if (byteCount == 0) {
      Serial.println("DIAGNOSTIC: No data seen on RX pin. Check wires!");
    } else {
      Serial.printf("DIAGNOSTIC: Bytes Recv: %u, Valid Packets: %u, Current Ch15: %u\n",
                    byteCount, packetCount, parser.getChannel(TARGET_CHANNEL - 1));
    }
    lastReport = millis();
  }
}
