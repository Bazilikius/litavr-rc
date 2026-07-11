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
#define PWM_PIN 10
#define LEDC_CHANNEL 0
#define TARGET_CHANNEL 15

CrsfParser parser;
PwmController controller(PWM_PIN, LEDC_CHANNEL);

void setup() {
  // Built-in USB Serial for debugging
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n--- ESP32-C3 CRSF to PWM Start ---");
  Serial.printf("Config: RX=%d, TX=%d, PWM=%d, Baud=%d\n", RX_PIN, TX_PIN, PWM_PIN, CRSF_BAUDRATE);

  // Initialize Hardware UART for CRSF
  CRSF_SERIAL.begin(CRSF_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initialize PWM Controller
  controller.begin();
  Serial.println("System Ready.");
}

uint32_t byteCount = 0;
uint32_t packetCount = 0;
uint32_t lastReport = 0;

void loop() {
  while (CRSF_SERIAL.available()) {
    uint8_t b = CRSF_SERIAL.read();
    byteCount++;

    if (parser.processByte(b)) {
      packetCount++;
      uint16_t ch15 = parser.getChannel(TARGET_CHANNEL - 1);
      controller.update(ch15);
    }
  }

  // Periodic diagnostic report every 2 seconds
  if (millis() - lastReport > 2000) {
    if (byteCount == 0) {
      Serial.println("WARNING: No data received on RX pin. Check wiring!");
    } else {
      uint16_t currentCh15 = parser.getChannel(TARGET_CHANNEL - 1);
      Serial.printf("Stats: Bytes Recv: %u, RC Packets: %u, Ch15: %u\n",
                    byteCount, packetCount, currentCh15);
    }
    lastReport = millis();
  }
}
