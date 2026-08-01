#include "PwmController.h"
#include <esp_arduino_version.h>

#define PWM_FREQ 50
#define PWM_RES 16 // 16-bit resolution for maximum precision (0 - 65535)

// LEDC channels for Core 2.x
#define LEFT_LEDC_CHAN 0
#define RIGHT_LEDC_CHAN 1

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin),
      _currentLeftUs(0.0f), _currentRightUs(0.0f), _leftVel(0.0f), _rightVel(0.0f), _lastUpdateMs(0) {}

void PwmController::begin(bool invertLeft, bool invertRight, uint16_t minUs, uint16_t maxUs) {
    // Other digital outputs
    pinMode(_extraPin, OUTPUT);
    pinMode(_ledPin, OUTPUT);

    // GPIO 25 is a limit switch input with pullup
    pinMode(_mosfetPin, INPUT_PULLUP);

    // Inputs with pullups
    pinMode(_upperSwPin, INPUT_PULLUP);
    pinMode(_lowerSwPin, INPUT_PULLUP);

    // Setup LEDC PWM on ESP32
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    // Arduino ESP32 Core 3.x APIs
    ledcAttach(_leftServoPin, PWM_FREQ, PWM_RES);
    ledcAttach(_rightServoPin, PWM_FREQ, PWM_RES);
#else
    // Arduino ESP32 Core 2.x APIs
    ledcSetup(LEFT_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_leftServoPin, LEFT_LEDC_CHAN);

    ledcSetup(RIGHT_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_rightServoPin, RIGHT_LEDC_CHAN);
#endif

    Serial.printf("[PWM] Initialized LEDC on LeftServo=%d, RightServo=%d | MosfetSwitchPin=%d, Extra=%d, LED=%d, UpperSw=%d, LowerSw=%d\n",
                  _leftServoPin, _rightServoPin, _mosfetPin, _extraPin, _ledPin, _upperSwPin, _lowerSwPin);

    // Startup test sequence: sweep servos slowly and organically to verify hardware (500us -> 2500us -> 500us)
    Serial.println("[PWM] Running synchronized boot-up diagnostics sweep (500us -> 2500us -> 500us)...");

    // Initialize starting positions
    _currentLeftUs = 500.0f;
    _currentRightUs = 500.0f;
    _leftVel = 0.0f;
    _rightVel = 0.0f;
    _lastUpdateMs = millis();

    // Step 1: Sweep slowly UP from 500us to 2500us using the organic PD motion profile
    Serial.println("[PWM] Sweeping UP (500 -> 2500)...");
    while (true) {
        updateServos(true, 500, 2500, invertLeft, invertRight);

        // Break once we are extremely close to the target of 2500us
        float leftTarget = invertLeft ? 500.0f : 2500.0f;
        if (abs(_currentLeftUs - leftTarget) < 1.0f) {
            break;
        }
        delay(15); // Smooth step delay
    }

    // Step 2: Sweep slowly DOWN from 2500us to 500us using the organic PD motion profile
    Serial.println("[PWM] Sweeping DOWN (2500 -> 500)...");
    while (true) {
        updateServos(false, 500, 2500, invertLeft, invertRight);

        // Break once we are extremely close to the target of 500us
        float leftTarget = invertLeft ? 2500.0f : 500.0f;
        if (abs(_currentLeftUs - leftTarget) < 1.0f) {
            break;
        }
        delay(15); // Smooth step delay
    }

    // Toggle Extra & LED
    digitalWrite(_extraPin, HIGH);
    digitalWrite(_ledPin, HIGH);
    delay(300);
    digitalWrite(_extraPin, LOW);
    digitalWrite(_ledPin, LOW);

    Serial.println("[PWM] Diagnostics complete. Outputs Ready.");
}

bool PwmController::isAnyLimitSwitchOpen() {
    // Under INPUT_PULLUP, a switch is open when it reads HIGH
    return (digitalRead(_upperSwPin) == HIGH) || (digitalRead(_lowerSwPin) == HIGH);
}

bool PwmController::isUpperSwitchOpen() {
    return (digitalRead(_upperSwPin) == HIGH);
}

bool PwmController::isMosfetSwPressed() {
    // Pressed limit switch connects GPIO 25 to GND, reading LOW
    return (digitalRead(_mosfetPin) == LOW);
}

void PwmController::updateServos(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertLeft, bool invertRight) {
    // Inversion of both servos implemented in exactly two lines of code:
    float leftTarget = (float)(invertLeft ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs));
    float rightTarget = (float)(invertRight ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs));

    leftTarget = constrain(leftTarget, 500.0f, 2500.0f);
    rightTarget = constrain(rightTarget, 500.0f, 2500.0f);

    // Initialize current positions on the first run
    if (_currentLeftUs == 0.0f || _currentRightUs == 0.0f) {
        _currentLeftUs = leftTarget;
        _currentRightUs = rightTarget;
        _leftVel = 0.0f;
        _rightVel = 0.0f;
        _lastUpdateMs = millis();
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _lastUpdateMs;

    if (elapsed > 0) {
        _lastUpdateMs = now;

        // --- Industrial-Grade Proportional-Derivative (PD) Trajectory Profile ---
        // Acceleration = (Spring Pull towards Target) - (Damping friction based on Velocity)
        // This naturally limits acceleration and jerk, creating beautiful smooth S-curve starts and stops.
        float Kp = 0.000004f; // Spring stiffness (lower = slower acceleration & softer starts)
        float Kd = 0.035f;    // Friction damping (prevents oscillation and ensures smooth deceleration)

        // Left Servo PD update
        float leftError = leftTarget - _currentLeftUs;
        float leftAccel = (leftError * Kp) - (Kd * _leftVel);
        _leftVel += leftAccel * elapsed;

        // Speed cap for slow, majestic movements (~500us to 2500us over 60-90 seconds)
        float maxVel = 0.025f;
        if (_leftVel > maxVel) _leftVel = maxVel;
        if (_leftVel < -maxVel) _leftVel = -maxVel;

        _currentLeftUs += _leftVel * elapsed;

        // Right Servo PD update
        float rightError = rightTarget - _currentRightUs;
        float rightAccel = (rightError * Kp) - (Kd * _rightVel);
        _rightVel += rightAccel * elapsed;

        if (_rightVel > maxVel) _rightVel = maxVel;
        if (_rightVel < -maxVel) _rightVel = -maxVel;

        _currentRightUs += _rightVel * elapsed;
    }

    writeMicros(_leftServoPin, (uint32_t)_currentLeftUs);
    writeMicros(_rightServoPin, (uint32_t)_currentRightUs);
}

void PwmController::updateOutputs(bool isActive, uint8_t offLevel) {
    uint8_t outputState;

    if (isActive) {
        // ON state is the opposite of offLevel
        outputState = (offLevel == 0) ? HIGH : LOW;
    } else {
        // OFF state is offLevel
        outputState = (offLevel == 0) ? LOW : HIGH;
    }

    digitalWrite(_extraPin, outputState);
}

void PwmController::writeMicros(uint8_t pin, uint32_t us) {
    // Convert microseconds to duty cycle (16-bit, 50Hz)
    // 50Hz period is 20000 microseconds. 16-bit max duty is 65535.
    uint32_t duty = (us * 65535) / 20000;

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    // Arduino ESP32 Core 3.x
    ledcWrite(pin, duty);
#else
    // Arduino ESP32 Core 2.x
    uint8_t chan = (pin == _leftServoPin) ? LEFT_LEDC_CHAN : RIGHT_LEDC_CHAN;
    ledcWrite(chan, duty);
#endif
}
