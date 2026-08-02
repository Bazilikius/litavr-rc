#ifndef PWM_CONTROLLER_H
#define PWM_CONTROLLER_H

#include <Arduino.h>

class PwmController {
public:
    PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin);
    void begin(bool invertLeft, bool invertRight, uint16_t minUs, uint16_t maxUs, bool isUpperTriggeredAtBoot);

    // Updates physical servo values based on activation state and configuration, respecting left/right inversions (using 2-line inversion logic with customizable speed limiting)
    void updateServos(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertLeft, bool invertRight, uint16_t speedUsPerSec);

    // Returns true if either Left or Right servo is currently in motion
    bool isServoMoving(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertLeft, bool invertRight);

    // Updates Extra Pin (GPIO 15) and MOSFET Pin (GPIO 26) values via 50Hz LEDC PWM signals
    void updatePwmOutputs(bool isMosfetActive, bool isExtraActive, uint8_t mosfetOffLevel);

    // Utility to write microsecond values to a pin using ESP32 ledc peripheral
    void writeMicros(uint8_t pin, uint32_t us);

private:
    uint8_t _leftServoPin;
    uint8_t _rightServoPin;
    uint8_t _mosfetPin;   // GPIO 26: MOSFET output (50Hz RC Switch PWM)
    uint8_t _extraPin;    // GPIO 15: Extra output (50Hz RC Switch PWM)
    uint8_t _ledPin;      // GPIO 27: LED indicator
    uint8_t _upperSwPin;  // GPIO 32: Upper Limit Switch
    uint8_t _lowerSwPin;  // GPIO 33: Lower Limit Switch

    // Smooth servo speed-limiting state variables
    float _currentLeftUs;
    float _currentRightUs;
    uint32_t _lastUpdateMs;
};

#endif
