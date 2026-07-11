#include "PwmController.h"

// For ESP32, we use LEDC for PWM
#define PWM_FREQ 50
#define PWM_RES 16 // 16-bit resolution (0-65535)

PwmController::PwmController(uint8_t pin, uint8_t channel) : _pin(pin), _channel(channel) {}

void PwmController::begin() {
    // New ESP32 LEDC API (Core 3.0+)
    // Frequency 50Hz, Resolution 16-bit
    if (!ledcAttach(_pin, PWM_FREQ, PWM_RES)) {
        // Fallback for older cores or specific errors
        // Note: ledcAttach returns bool in newer cores
    }

    // Initial state: Center (1500us)
    update(992);
}

void PwmController::update(uint16_t crsfValue) {
    // Standard CRSF values are 172 (1000us) to 1811 (2000us)
    // Map to PWM Pulse Width in microseconds
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);

    // Safety constrain to RC limits
    pwm_us = constrain(pwm_us, 500, 2500);

    // Convert microseconds to LEDC duty cycle
    // Period = 1,000,000 / 50Hz = 20,000 us
    // Duty = (PulseWidth / Period) * (2^Resolution - 1)
    // For 16-bit: (pwm_us * 65535) / 20000
    uint32_t duty = (pwm_us * 65535) / 20000;

    ledcWrite(_pin, duty);
}
