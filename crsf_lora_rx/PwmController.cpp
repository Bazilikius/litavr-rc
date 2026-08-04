#include "PwmController.h"
#include <esp_arduino_version.h>

#define PWM_FREQ 50
#define PWM_RES 14 // 14-bit resolution for maximum ESP32 clock division compatibility (0 - 16383)

// LEDC channels for Core 2.x
#define LEFT_LEDC_CHAN 0
#define RIGHT_LEDC_CHAN 1
#define EXTRA_LEDC_CHAN 2
#define MOSFET_LEDC_CHAN 3

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin),
      _currentLeftUs(0.0f), _currentRightUs(0.0f), _lastUpdateMs(0) {}

void PwmController::begin(bool invertLeft, bool invertRight, uint16_t minUs, uint16_t maxUs, bool isUpperTriggeredAtBoot) {
    // Explicitly configure output pins
    pinMode(_ledPin, OUTPUT);
    pinMode(_leftServoPin, OUTPUT);
    pinMode(_rightServoPin, OUTPUT);
    pinMode(_extraPin, OUTPUT);
    pinMode(_mosfetPin, OUTPUT);

    // Initialize physical indicators: Red LED on GPIO 21, Blue LED on GPIO 22
    pinMode(21, OUTPUT);
    pinMode(22, OUTPUT);
    digitalWrite(21, HIGH); // Red ON (booting/moving)
    digitalWrite(22, LOW);  // Blue OFF

    // Inputs with pullups
    pinMode(_upperSwPin, INPUT_PULLUP);
    pinMode(_lowerSwPin, INPUT_PULLUP);

    // Setup LEDC PWM on ESP32 for Servos (Left Servo on GPIO 4, Right Servo on GPIO 13) and RC Switch Outputs (GPIO 26 and GPIO 15)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    // Arduino ESP32 Core 3.x APIs
    ledcAttach(_leftServoPin, PWM_FREQ, PWM_RES);
    ledcAttach(_rightServoPin, PWM_FREQ, PWM_RES);
    ledcAttach(_extraPin, PWM_FREQ, PWM_RES);
    ledcAttach(_mosfetPin, PWM_FREQ, PWM_RES);
#else
    // Arduino ESP32 Core 2.x APIs
    ledcSetup(LEFT_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_leftServoPin, LEFT_LEDC_CHAN);

    ledcSetup(RIGHT_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_rightServoPin, RIGHT_LEDC_CHAN);

    ledcSetup(EXTRA_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_extraPin, EXTRA_LEDC_CHAN);

    ledcSetup(MOSFET_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_mosfetPin, MOSFET_LEDC_CHAN);
#endif

    // Set starting positions organically based on the Upper Limit Switch state at boot!
    if (isUpperTriggeredAtBoot) {
        _currentLeftUs = (float)(invertLeft ? minUs : maxUs);
        _currentRightUs = (float)(invertRight ? minUs : maxUs);
    } else {
        _currentLeftUs = (float)(invertLeft ? maxUs : minUs);
        _currentRightUs = (float)(invertRight ? maxUs : minUs);
    }

    _lastUpdateMs = millis();

    // Startup test sequence: sweep servos from 1000us to 2000us and back on boot to verify physical connectivity
    for (uint32_t us = 1000; us <= 2000; us += 20) {
        writeMicros(_leftServoPin, us);
        writeMicros(_rightServoPin, us);
        delay(10);
    }
    delay(200);
    for (uint32_t us = 2000; us >= 1000; us -= 20) {
        writeMicros(_leftServoPin, us);
        writeMicros(_rightServoPin, us);
        delay(10);
    }
    delay(200);

    // Toggle LED
    digitalWrite(_ledPin, HIGH);
    delay(300);
    digitalWrite(_ledPin, LOW);

    // Initial state: stationary (Blue ON, Red OFF)
    digitalWrite(21, LOW);
    digitalWrite(22, HIGH);
}

void PwmController::updateServos(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertLeft, bool invertRight, uint16_t speedUsPerSec) {
    // Inversion of both servos implemented in exactly two lines of code:
    float leftTarget = (float)(invertLeft ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs));
    float rightTarget = (float)(invertRight ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs));

    leftTarget = constrain(leftTarget, 500.0f, 2500.0f);
    rightTarget = constrain(rightTarget, 500.0f, 2500.0f);

    // Initialize current positions to target on the first run if uninitialized
    if (_currentLeftUs == 0.0f || _currentRightUs == 0.0f) {
        _currentLeftUs = leftTarget;
        _currentRightUs = rightTarget;
        _lastUpdateMs = millis();
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _lastUpdateMs;

    if (elapsed > 0) {
        _lastUpdateMs = now;

        // Custom slew-rate step based on speedUsPerSec (us per millisecond)
        float maxStep = (float)elapsed * ((float)speedUsPerSec / 1000.0f);

        // Smoothly adjust Left Servo
        if (_currentLeftUs < leftTarget) {
            _currentLeftUs += maxStep;
            if (_currentLeftUs > leftTarget) _currentLeftUs = leftTarget;
        } else if (_currentLeftUs > leftTarget) {
            _currentLeftUs -= maxStep;
            if (_currentLeftUs < leftTarget) _currentLeftUs = leftTarget;
        }

        // Smoothly adjust Right Servo
        if (_currentRightUs < rightTarget) {
            _currentRightUs += maxStep;
            if (_currentRightUs > rightTarget) _currentRightUs = rightTarget;
        } else if (_currentRightUs > rightTarget) {
            _currentRightUs -= maxStep;
            if (_currentRightUs < rightTarget) _currentRightUs = rightTarget;
        }
    }

    writeMicros(_leftServoPin, (uint32_t)_currentLeftUs);
    writeMicros(_rightServoPin, (uint32_t)_currentRightUs);
}

bool PwmController::isServoMoving(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertLeft, bool invertRight) {
    float leftTarget = (float)(invertLeft ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs));
    float rightTarget = (float)(invertRight ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs));

    return (abs(_currentLeftUs - leftTarget) > 1.0f) || (abs(_currentRightUs - rightTarget) > 1.0f);
}

void PwmController::updatePwmOutputs(bool isMosfetActive, bool isExtraActive, uint8_t mosfetOffLevel, uint8_t powerKeyOffLevel, uint16_t mosfetActiveWidth) {
    uint32_t activeMosfetUs = (mosfetActiveWidth == 2500) ? 2500 : 2000; // Selectable 2000us or 2500us output signal
    uint32_t inactiveMosfetUs = (mosfetOffLevel == 0) ? 1000 : 2000;

    uint32_t activeExtraUs = 2000;  // Extra Pin / Power Key Active target: 2000us PWM
    uint32_t inactiveExtraUs = (powerKeyOffLevel == 0) ? 1000 : 2000;

    writeMicros(_mosfetPin, isMosfetActive ? activeMosfetUs : inactiveMosfetUs);
    writeMicros(_extraPin, isExtraActive ? activeExtraUs : inactiveExtraUs);
}

void PwmController::writeMicros(uint8_t pin, uint32_t us) {
    // Convert microseconds to duty cycle (14-bit, 50Hz)
    // 50Hz period is 20000 microseconds. 14-bit max duty is 16383.
    uint32_t duty = (us * 16383) / 20000;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    // Arduino ESP32 Core 3.x
    ledcWrite(pin, duty);
#else
    // Arduino ESP32 Core 2.x
    uint8_t chan;
    if (pin == _leftServoPin) chan = LEFT_LEDC_CHAN;
    else if (pin == _rightServoPin) chan = RIGHT_LEDC_CHAN;
    else if (pin == _extraPin) chan = EXTRA_LEDC_CHAN;
    else chan = MOSFET_LEDC_CHAN;
    ledcWrite(chan, duty);
#endif
}
