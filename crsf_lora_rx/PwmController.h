#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>

class PwmController {
public:
    PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin, uint8_t powerKeyPin);
    void begin(bool invertLeft, bool invertRight, uint16_t minUs, uint16_t maxUs, bool isUpperTriggeredAtBoot);

    // Reads limit switches, returns true if at least one is open (reads HIGH)
    bool isAnyLimitSwitchOpen();

    // Returns true if upper switch is open (reads HIGH)
    bool isUpperSwitchOpen();

    // Returns true if the switch on GPIO 25 (former mosfet pin) is pressed (reads LOW)
    bool isMosfetSwPressed();

    // Updates physical servo values based on activation state and configuration, respecting left/right inversions (using 2-line inversion logic with smooth speed limiting)
    void updateServos(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertLeft, bool invertRight);

    // Updates Extra Pin (GPIO 26) and Power Key Pin (GPIO 15) values via 50Hz PWM signals
    void updatePwmOutputs(bool isExtraActive, bool isPowerKeyActive, uint8_t extraOffLevel, uint8_t powerKeyOffLevel);

    // Utility to write microsecond values to a pin using ESP32 ledc peripheral
    void writeMicros(uint8_t pin, uint32_t us);

private:
    uint8_t _leftServoPin;
    uint8_t _rightServoPin;
    uint8_t _mosfetPin; // This pin is configured as INPUT_PULLUP (limit switch to move servo UP)
    uint8_t _extraPin;  // Configured as a 50Hz LEDC PWM output (RC Switch)
    uint8_t _ledPin;
    uint8_t _upperSwPin;
    uint8_t _lowerSwPin;
    uint8_t _powerKeyPin; // Configured as a 50Hz LEDC PWM output (Power Key)

    // Smooth servo speed-limiting state variables (using floats for high precision sub-microsecond rate-limiting)
    float _currentLeftUs;
    float _currentRightUs;
    uint32_t _lastUpdateMs;
};

#endif
