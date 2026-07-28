#include "PwmController.h"

#define PWM_FREQ 50
#define PWM_RES 12

PwmController::PwmController(uint8_t switchPin, uint8_t servoPin)
    : _switchPin(switchPin), _servoPin(servoPin) {}

void PwmController::begin() {
    analogWriteFrequency(_switchPin, PWM_FREQ);
    analogWriteResolution(_switchPin, PWM_RES);

    analogWriteFrequency(_servoPin, PWM_FREQ);
    analogWriteResolution(_servoPin, PWM_RES);

    Serial.printf("PWM Outputs Initialized: Switch Pin %d, Servo Pin %d\n", _switchPin, _servoPin);

    // Sweeping test sequence at boot to verify physical connections
    Serial.println("Testing Outputs...");
    writeMicros(_switchPin, 1000);
    writeMicros(_servoPin, 1000);
    delay(500);
    writeMicros(_switchPin, 1500);
    writeMicros(_servoPin, 1500);
    delay(500);
    writeMicros(_switchPin, 2000);
    writeMicros(_servoPin, 2000);
    delay(500);
    writeMicros(_switchPin, 1500);
    writeMicros(_servoPin, 1500);
    Serial.println("Outputs Ready.");
}

void PwmController::writeMicros(uint8_t pin, uint32_t us) {
    // Convert microseconds to duty cycle (12-bit, 50Hz)
    uint32_t duty = (us * 4095) / 20000;
    analogWrite(pin, duty);
}

void PwmController::updateSwitch(uint16_t crsfValue) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);
    pwm_us = constrain(pwm_us, 800, 2200);
    writeMicros(_switchPin, pwm_us);
}

void PwmController::updateServo(uint16_t crsfValue) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, PWM_MIN, PWM_MAX);
    pwm_us = constrain(pwm_us, 800, 2200);
    writeMicros(_servoPin, pwm_us);
}
