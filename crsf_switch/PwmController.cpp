#include "PwmController.h"

// For ESP32, we use LEDC for PWM
// 50Hz frequency for RC PWM
#define PWM_FREQ 50
#define PWM_RES 16 // 16-bit resolution

PwmController::PwmController(uint8_t pin, uint8_t channel) : _pin(pin), _channel(channel) {}

void PwmController::begin() {
    // New ESP32 LEDC API (Core 3.0+)
    ledcAttach(_pin, PWM_FREQ, PWM_RES);
}

void PwmController::update(uint16_t crsfValue) {
    // Map CRSF (172-1811) to PWM Pulse Width (1000us-2000us)
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);
    pwm_us = constrain(pwm_us, PWM_MIN, PWM_MAX);

    // Convert microseconds to LEDC duty cycle
    // Duty = (PulseWidth / Period) * (2^Resolution - 1)
    // Period = 1,000,000 / Frequency = 20,000 us
    // Using 65535 for 16-bit resolution
    uint32_t duty = (pwm_us * 65535) / 20000;
    ledcWrite(_pin, duty);
}
