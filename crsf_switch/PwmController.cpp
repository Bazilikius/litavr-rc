#include "PwmController.h"

PwmController::PwmController(uint8_t switchPin, uint8_t servoPin, uint8_t cameraPin)
    : _switchPin(switchPin), _servoPin(servoPin), _cameraPin(cameraPin) {}

void PwmController::begin() {
    // Standard ESP32Servo attachment setup
    // Allocate hardware timers and attach pins (defaults: min 544us, max 2400us)
    _switchServo.attach(_switchPin, 500, 2500);
    _servoServo.attach(_servoPin, 500, 2500);
    _cameraServo.attach(_cameraPin, 500, 2500);

    Serial.printf("ESP32Servo attached: Switch %d, Servo %d, Camera %d\n", _switchPin, _servoPin, _cameraPin);

    // Sweeping test sequence at boot to verify physical connections (using general 1000us-2000us)
    Serial.println("Testing Outputs (ESP32Servo)...");
    writeMicros(_switchServo, 1000);
    writeMicros(_servoServo, 1000);
    writeMicros(_cameraServo, 1000);
    delay(500);
    writeMicros(_switchServo, 1500);
    writeMicros(_servoServo, 1500);
    writeMicros(_cameraServo, 1500);
    delay(500);
    writeMicros(_switchServo, 2000);
    writeMicros(_servoServo, 2000);
    writeMicros(_cameraServo, 2000);
    delay(500);
    writeMicros(_switchServo, 1500);
    writeMicros(_servoServo, 1500);
    writeMicros(_cameraServo, 1500);
    Serial.println("Outputs Ready.");
}

void PwmController::writeMicros(Servo &servoObj, uint32_t us) {
    // Write pulse width directly in microseconds
    servoObj.writeMicroseconds(us);
}

void PwmController::updateSwitch(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, minUs, maxUs);
    pwm_us = constrain(pwm_us, 500, 2500);
    writeMicros(_switchServo, pwm_us);
}

void PwmController::updateServo(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, minUs, maxUs);
    pwm_us = constrain(pwm_us, 500, 2500);
    writeMicros(_servoServo, pwm_us);
}

void PwmController::updateCamera(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs) {
    uint32_t pwm_us = map(crsfValue, CRSF_MIN, CRSF_MAX, minUs, maxUs);
    pwm_us = constrain(pwm_us, 500, 2500);
    writeMicros(_cameraServo, pwm_us);
}
