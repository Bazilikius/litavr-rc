/*
 * CRSF to PWM Switch Controller for ESP32
 * Listens to Channel 15 from CRSF stream and controls a PWM output.
 */

#include "CrsfParser.h"
#include "PwmController.h"

// Configuration
#define CRSF_SERIAL Serial1
#define CRSF_BAUDRATE 400000
#define RX_PIN 2
#define TX_PIN 3
#define PWM_PIN 4
#define LEDC_CHANNEL 0
#define TARGET_CHANNEL 15 // 1-indexed

CrsfParser parser;
PwmController controller(PWM_PIN, LEDC_CHANNEL);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting CRSF to PWM Switch Controller...");

  // Initialize CRSF Serial
  CRSF_SERIAL.begin(CRSF_BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // Initialize PWM Controller
  controller.begin();
}

void loop() {
  while (CRSF_SERIAL.available()) {
    uint8_t b = CRSF_SERIAL.read();
    if (parser.processByte(b)) {
      // Valid RC packet received
      uint16_t ch15 = parser.getChannel(TARGET_CHANNEL - 1);
      controller.update(ch15);

      // Optional: Debugging
      // static uint32_t lastPrint = 0;
      // if (millis() - lastPrint > 500) {
      //   Serial.print("Ch15: "); Serial.print(ch15);
      //   Serial.print(" -> PWM updated");
      //   Serial.println();
      //   lastPrint = millis();
      // }
    }
  }
}
