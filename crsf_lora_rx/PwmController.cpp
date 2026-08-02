#include "PwmController.h"
#include <esp_arduino_version.h>

#define PWM_FREQ 50
#define PWM_RES 16 // 16-bit resolution for maximum precision (0 - 65535)

// LEDC channels for Core 2.x
#define LEFT_LEDC_CHAN 0
#define RIGHT_LEDC_CHAN 1
#define EXTRA_LEDC_CHAN 2
#define POWER_LEDC_CHAN 3

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin, uint8_t powerKeyPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin), _powerKeyPin(powerKeyPin),
      _currentLeftUs(0.0f), _currentRightUs(0.0f), _lastUpdateMs(0) {}

void PwmController::begin(bool invertLeft, bool invertRight, uint16_t minUs, uint16_t maxUs, bool isUpperTriggeredAtBoot) {
    // Other digital outputs
    pinMode(_extraPin, OUTPUT);
    pinMode(_ledPin, OUTPUT);

    // Initialize physical indicators: Red LED on GPIO 21, Blue LED on GPIO 22
    pinMode(21, OUTPUT);
    pinMode(22, OUTPUT);
    digitalWrite(21, HIGH); // Red ON (booting/moving)
    digitalWrite(22, LOW);  // Blue OFF

    // GPIO 25 is a limit switch input with pullup
    pinMode(_mosfetPin, INPUT_PULLUP);

    // Inputs with pullups
    pinMode(_upperSwPin, INPUT_PULLUP);
    pinMode(_lowerSwPin, INPUT_PULLUP);

    // Setup LEDC PWM on ESP32 for all 4 channels (Left Servo, Right Servo, Extra Pin, Power Key)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    // Arduino ESP32 Core 3.x APIs
    ledcAttach(_leftServoPin, PWM_FREQ, PWM_RES);
    ledcAttach(_rightServoPin, PWM_FREQ, PWM_RES);
    ledcAttach(_extraPin, PWM_FREQ, PWM_RES);
    ledcAttach(_powerKeyPin, PWM_FREQ, PWM_RES);
#else
    // Arduino ESP32 Core 2.x APIs
    ledcSetup(LEFT_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_leftServoPin, LEFT_LEDC_CHAN);

    ledcSetup(RIGHT_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_rightServoPin, RIGHT_LEDC_CHAN);

    ledcSetup(EXTRA_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_extraPin, EXTRA_LEDC_CHAN);

    ledcSetup(POWER_LEDC_CHAN, PWM_FREQ, PWM_RES);
    ledcAttachPin(_powerKeyPin, POWER_LEDC_CHAN);
#endif

    Serial.printf("[PWM] Initialized LEDC on LeftServo=%d, RightServo=%d, Extra=%d, PowerKey=%d | MosfetSwitchPin=%d, LED=%d, UpperSw=%d, LowerSw=%d\n",
                  _leftServoPin, _rightServoPin, _extraPin, _powerKeyPin, _mosfetPin, _ledPin, _upperSwPin, _lowerSwPin);

    // Set starting positions organically based on the Upper Limit Switch state at boot!
    // This prevents any unnecessary travel, jumps, or jerks on startup!
    if (isUpperTriggeredAtBoot) {
        Serial.println("[PWM] Upper Limit Switch triggered at boot. Initializing servos in active/UP position.");
        _currentLeftUs = (float)(invertLeft ? minUs : maxUs);
        _currentRightUs = (float)(invertRight ? minUs : maxUs);
    } else {
        Serial.println("[PWM] Upper Limit Switch not triggered. Initializing servos in inactive/DOWN position.");
        _currentLeftUs = (float)(invertLeft ? maxUs : minUs);
        _currentRightUs = (float)(invertRight ? maxUs : minUs);
    }

    _lastUpdateMs = millis();

    // Startup test sequence: sweep servos slowly and synchronously to verify hardware (1500us -> 2200us -> 1500us)
    Serial.println("[PWM] Running synchronized boot-up diagnostics sweep (1500us -> 2200us -> 1500us)...");

    // Step 1: Sweep UP from 1500us to 2200us (with a safety timeout to prevent boot hang)
    Serial.println("[PWM] Sweeping UP (1500 -> 2200)...");
    uint32_t safetyCount = 0;
    while (safetyCount < 300) {
        updateServos(true, 1500, 2200, invertLeft, invertRight);

        // Break once Left Servo reaches its active target (taking inversion into account)
        float leftTarget = invertLeft ? 1500.0f : 2200.0f;
        if (abs(_currentLeftUs - leftTarget) < 2.0f) {
            break;
        }
        delay(15); // Smooth step delay
        safetyCount++;
    }

    // Step 2: Sweep DOWN from 2200us to 1500us (with a safety timeout to prevent boot hang)
    Serial.println("[PWM] Sweeping DOWN (2200 -> 1500)...");
    safetyCount = 0;
    while (safetyCount < 300) {
        updateServos(false, 1500, 2200, invertLeft, invertRight);

        // Break once Left Servo reaches its inactive target (taking inversion into account)
        float leftTarget = invertLeft ? 2200.0f : 1500.0f;
        if (abs(_currentLeftUs - leftTarget) < 2.0f) {
            break;
        }
        delay(15); // Smooth step delay
        safetyCount++;
    }

    // Toggle Extra & LED
    digitalWrite(_ledPin, HIGH);
    delay(300);
    digitalWrite(_ledPin, LOW);

    // Initial state: stationary (Blue ON, Red OFF)
    digitalWrite(21, LOW);
    digitalWrite(22, HIGH);

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

        // --- Fast, Symmetric Slew-Rate Limiter (Constant speed in both directions) ---
        // Speed: 0.35us per millisecond. Traveling the full 700us range (1500 to 2200) takes exactly 1.75 seconds.
        float maxStep = (float)elapsed * 0.35f;

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

    // Toggle Red/Blue LEDs to indicate motion: Red (GPIO 21) = moving, Blue (GPIO 22) = completed/stationary
    bool moving = (abs(_currentLeftUs - leftTarget) > 0.5f) || (abs(_currentRightUs - rightTarget) > 0.5f);
    if (moving) {
        digitalWrite(21, HIGH); // Red ON
        digitalWrite(22, LOW);  // Blue OFF
    } else {
        digitalWrite(21, LOW);  // Red OFF
        digitalWrite(22, HIGH); // Blue ON
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
    uint8_t chan;
    if (pin == _leftServoPin) chan = LEFT_LEDC_CHAN;
    else if (pin == _rightServoPin) chan = RIGHT_LEDC_CHAN;
    else if (pin == _extraPin) chan = EXTRA_LEDC_CHAN;
    else chan = POWER_LEDC_CHAN;
    ledcWrite(chan, duty);
#endif
}
