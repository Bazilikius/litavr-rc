#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>

class PwmController {
public:
    PwmController(uint8_t switchPin, uint8_t servoPin);
    void begin();
    void updateSwitch(uint16_t crsfValue);
    void updateServo(uint16_t crsfValue);
    void writeMicros(uint8_t pin, uint32_t us);

private:
    uint8_t _switchPin;
    uint8_t _servoPin;

    static const uint16_t CRSF_MIN = 172;
    static const uint16_t CRSF_MAX = 1811;
    static const uint16_t PWM_MIN = 1000; // us
    static const uint16_t PWM_MAX = 2000; // us
};

#endif
