#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>

class PwmController {
public:
    PwmController(uint8_t switchPin, uint8_t servoPin, uint8_t cameraPin);
    void begin();
    void updateSwitch(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs);
    void updateServo(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs);
    void updateCamera(uint16_t crsfValue, uint16_t minUs, uint16_t maxUs);
    void writeMicros(uint8_t pin, uint32_t us);

private:
    uint8_t _switchPin;
    uint8_t _servoPin;
    uint8_t _cameraPin;

    static const uint16_t CRSF_MIN = 172;
    static const uint16_t CRSF_MAX = 1811;
};

#endif
