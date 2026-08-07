#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class PwmController {
public:
    PwmController(uint8_t switchPin, uint8_t servoPin, uint8_t cameraPin);
    void begin();
    void updateSwitch(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs);
    void updateServo(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs);
    void updateCamera(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs);
    void writeMicros(Servo &servoObj, uint32_t us);

private:
    uint8_t _switchPin;
    uint8_t _servoPin;
    uint8_t _cameraPin;

    Servo _switchServo;
    Servo _servoServo;
    Servo _cameraServo;

    static const uint16_t CRSF_MIN = 172;
    static const uint16_t CRSF_MAX = 1811;
};

#endif
