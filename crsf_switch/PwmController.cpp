#include "PwmController.h"

// For ESP32-C3 with Arduino Core 3.0+
#define PWM_FREQ 50
#define PWM_RES 16 // Use 16-bit resolution (0-65535)

PwmController::PwmController(uint8_t pin, uint8_t channel) : _pin(pin), _channel(channel) {}

void PwmController::begin() {
    // New ESP32 LEDC API (Core 3.0+)
    if (!ledcAttach(_pin, PWM_FREQ, PWM_RES)) {
        Serial.println("PWM Init Failed!");
    }

    // Boot-up test sequence: Sweep to show PWM is working
    Serial.println("PWM Hardware Test Sweep...");
    writeMicros(1000);
    delay(500);
    writeMicros(1500);
    delay(500);
    writeMicros(2000);
    delay(500);
    writeMicros(1500);
    Serial.println("PWM Test Complete.");
}

void PwmController::writeMicros(uint32_t us) {
    // Convert microseconds to LEDC duty cycle
    // Period = 1,000,000 / 50Hz = 20,000 us
    // Duty = (PulseWidth / Period) * (2^Resolution - 1)
    // For 16-bit: (us * 65535) / 20000
    uint32_t duty = (us * 65535) / 20000;
    ledcWrite(_pin, duty);
}

void PwmController::update(uint16_t crsfValue) {
    // Map CRSF (172-1811) to PWM Pulse Width (1000us-2000us)
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);

    // Safety constrain
    pwm_us = constrain(pwm_us, 800, 2200);

    // Update PWM
    writeMicros(pwm_us);
}
