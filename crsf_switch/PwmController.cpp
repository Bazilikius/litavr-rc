#include "PwmController.h"

#define PWM_FREQ 50
#define PWM_RES 12

PwmController::PwmController(uint8_t switchPin, uint8_t servoPin, uint8_t cameraPin)
    : _switchPin(switchPin), _servoPin(servoPin), _cameraPin(cameraPin) {}

void PwmController::begin() {
    analogWriteFrequency(_switchPin, PWM_FREQ);
    analogWriteResolution(_switchPin, PWM_RES);

    analogWriteFrequency(_servoPin, PWM_FREQ);
    analogWriteResolution(_servoPin, PWM_RES);

    analogWriteFrequency(_cameraPin, PWM_FREQ);
    analogWriteResolution(_cameraPin, PWM_RES);

    Serial.printf("PWM Outputs Initialized: Switch %d, Servo %d, Camera %d\n", _switchPin, _servoPin, _cameraPin);

    // Sweeping test sequence at boot to verify physical connections (using general 1000us-2000us)
    Serial.println("Testing Outputs...");
    writeMicros(_switchPin, 1000);
    writeMicros(_servoPin, 1000);
    writeMicros(_cameraPin, 1000);
    delay(500);
    writeMicros(_switchPin, 1500);
    writeMicros(_servoPin, 1500);
    writeMicros(_cameraPin, 1500);
    delay(500);
    writeMicros(_switchPin, 2000);
    writeMicros(_servoPin, 2000);
    writeMicros(_cameraPin, 2000);
    delay(500);
    writeMicros(_switchPin, 1500);
    writeMicros(_servoPin, 1500);
    writeMicros(_cameraPin, 1500);
    Serial.println("Outputs Ready.");
}

void PwmController::writeMicros(uint8_t pin, uint32_t us) {
    // Convert microseconds to duty cycle (12-bit, 50Hz)
    uint32_t duty = (us * 4095) / 20000;
    analogWrite(pin, duty);
}

void PwmController::updateSwitch(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, minUs, maxUs);
    pwm_us = constrain(pwm_us, 500, 2500);
    writeMicros(_switchPin, pwm_us);
}

void PwmController::updateServo(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, minUs, maxUs);
    pwm_us = constrain(pwm_us, 500, 2500);
    writeMicros(_servoPin, pwm_us);
}

void PwmController::updateCamera(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, minUs, maxUs);
    pwm_us = constrain(pwm_us, 500, 2500);
    writeMicros(_cameraPin, pwm_us);
}
