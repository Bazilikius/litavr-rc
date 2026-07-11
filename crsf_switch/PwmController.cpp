#include "PwmController.h"

// Using 50Hz for RC components
#define PWM_FREQ 50
// 12-bit resolution (0-4095)
#define PWM_RES 12

PwmController::PwmController(uint8_t pin, uint8_t channel) : _pin(pin), _channel(channel) {}

void PwmController::begin() {
    // analogWriteFrequency and Resolution are global for the pin
    analogWriteFrequency(_pin, PWM_FREQ);
    analogWriteResolution(PWM_RES);

    Serial.println("PWM Hardware Initialized (analogWrite).");

    // Boot-up test sequence: Sweep to show PWM is working
    Serial.println("Starting PWM Sweep Test...");
    writeMicros(1000);
    delay(1000);
    writeMicros(1500);
    delay(1000);
    writeMicros(2000);
    delay(1000);
    writeMicros(1500);
    Serial.println("PWM Sweep Test Finished.");
}

void PwmController::writeMicros(uint32_t us) {
    // Convert microseconds to duty cycle
    // Period = 1,000,000 / 50Hz = 20,000 us
    // Duty = (us / 20000) * (2^12 - 1)
    // (us * 4095) / 20000
    uint32_t duty = (us * 4095) / 20000;

    analogWrite(_pin, duty);
}

void PwmController::update(uint16_t crsfValue) {
    // Map CRSF (172-1811) to PWM Pulse Width (1000us-2000us)
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);

    // Safety constrain
    pwm_us = constrain(pwm_us, 800, 2200);

    // Update PWM
    writeMicros(pwm_us);
}
