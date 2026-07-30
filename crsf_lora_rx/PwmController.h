#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>

class PwmController {
public:
    PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin);
    void begin();

    // Reads limit switches, returns true if at least one is open (reads HIGH)
    bool isAnyLimitSwitchOpen();

    // Returns true if upper switch is open (reads HIGH)
    bool isUpperSwitchOpen();

    // Updates physical servo values based on activation state and configuration
    void updateServos(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertRight);

    // Updates MOSFET and Extra Pin values
    void updateOutputs(bool isActive, uint8_t offLevel);

    // Utility to write microsecond values to a pin using analogWrite
    void writeMicros(uint8_t pin, uint32_t us);

private:
    uint8_t _leftServoPin;
    uint8_t _rightServoPin;
    uint8_t _mosfetPin;
    uint8_t _extraPin;
    uint8_t _ledPin;
    uint8_t _upperSwPin;
    uint8_t _lowerSwPin;
};

#endif
