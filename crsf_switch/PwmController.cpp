#include "PwmController.h"

// For ESP32-C3 with Arduino Core 3.0+
#define PWM_FREQ 50
#define PWM_RES 14 // 14-bit resolution is ample for 50Hz

PwmController::PwmController(uint8_t pin, uint8_t channel) : _pin(pin), _channel(channel) {}

void PwmController::begin() {
    // New ESP32 LEDC API (Core 3.0+)
    // Frequency 50Hz
    if (!ledcAttach(_pin, PWM_FREQ, PWM_RES)) {
        Serial.println("PWM Init Failed!");
    }

    // Boot-up test sequence: Sweep to show PWM is working
    Serial.println("PWM Hardware Test...");
    ledcWriteMicroseconds(_pin, 1000);
    delay(500);
    ledcWriteMicroseconds(_pin, 1500);
    delay(500);
    ledcWriteMicroseconds(_pin, 2000);
    delay(500);
    ledcWriteMicroseconds(_pin, 1500);
    Serial.println("PWM Test Complete.");
}

void PwmController::update(uint16_t crsfValue) {
    // Map CRSF (172-1811) to PWM Pulse Width (1000us-2000us)
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);

    // Safety constrain
    pwm_us = constrain(pwm_us, 800, 2200);

    // Directly write microseconds for high precision
    ledcWriteMicroseconds(_pin, pwm_us);
}
