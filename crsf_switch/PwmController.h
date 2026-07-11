#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>

class PwmController {
public:
    PwmController(uint8_t pin, uint8_t channel);
    void begin();
    void update(uint16_t crsfValue);
    void writeMicros(uint32_t us);

private:
    uint8_t _pin;
    uint8_t _channel;

    static const uint16_t CRSF_MIN = 172;
    static const uint16_t CRSF_MAX = 1811;
    static const uint16_t PWM_MIN = 1000; // us
    static const uint16_t PWM_MAX = 2000; // us
};

#endif
