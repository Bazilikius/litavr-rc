#include "PwmController.h"
#include <esp_arduino_version.h>

#define PWM_FREQ 50
#define PWM_RES 16 // 16-bit resolution for maximum precision (0 - 65535)

// LEDC channels for Core 2.x
#define LEFT_LEDC_CHAN 0
#define RIGHT_LEDC_CHAN 1

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin),
      _currentLeftUs(0.0f), _currentRightUs(0.0f), _lastUpdateMs(0) {}

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

    // Startup test sequence: sweep servos slowly and synchronously to verify hardware
    Serial.println("[PWM] Running synchronized boot-up diagnostics sweep...");

    // Sweep: Neutral -> Active -> Neutral (Since sweep is at boot and delay is short, we let them jump slightly faster for testing or keep them slow)
    updateServos(false, minUs, maxUs, invertLeft, invertRight); // Inactive / Neutral (DOWN)
    delay(500);
    _currentLeftUs = invertLeft ? minUs : maxUs;
    _currentRightUs = invertRight ? minUs : maxUs;
    updateServos(true, minUs, maxUs, invertLeft, invertRight);  // Active / UP
    delay(500);
    _currentLeftUs = invertLeft ? maxUs : minUs;
    _currentRightUs = invertRight ? maxUs : minUs;
    updateServos(false, minUs, maxUs, invertLeft, invertRight); // Inactive / Neutral (DOWN)
    delay(500);

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
        _lastUpdateMs = millis();
    }

    uint32_t now = millis();
    uint32_t elapsed = now - _lastUpdateMs;

    if (elapsed > 0) {
        _lastUpdateMs = now;

        // Controlled speed step: 500us change per 60,000ms (500us range over exactly 60 seconds)
        // Rate: 500.0f / 60000.0f = 1.0f / 120.0f = 0.008333333f microseconds per millisecond.
        float maxStep = (float)elapsed * (1.0f / 120.0f);

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
