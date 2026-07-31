#include "PwmController.h"
#include <esp_arduino_version.h>

#define PWM_FREQ 50
#define PWM_RES 16 // 16-bit resolution for maximum precision (0 - 65535)

// LEDC channels for Core 2.x
#define LEFT_LEDC_CHAN 0
#define RIGHT_LEDC_CHAN 1

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin) {}

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

    // Startup test sequence: sweep servos synchronously and toggle outputs to verify hardware
    Serial.println("[PWM] Running synchronized boot-up diagnostics sweep...");

    // Sweep: Neutral -> Active -> Neutral
    updateServos(false, minUs, maxUs, invertLeft, invertRight); // Inactive / Neutral (DOWN)
    delay(500);
    updateServos(true, minUs, maxUs, invertLeft, invertRight);  // Active / UP
    delay(500);
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
    uint32_t leftVal = invertLeft ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs);
    uint32_t rightVal = invertRight ? (isActive ? minUs : maxUs) : (isActive ? maxUs : minUs);

    leftVal = constrain(leftVal, 500, 2500);
    rightVal = constrain(rightVal, 500, 2500);

    writeMicros(_leftServoPin, leftVal);
    writeMicros(_rightServoPin, rightVal);
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
