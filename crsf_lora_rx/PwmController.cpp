#include "PwmController.h"

#define PWM_FREQ 50
#define PWM_RES 12

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin) {}

void PwmController::begin(bool invertLeft, bool invertRight, uint16_t minUs, uint16_t maxUs) {
    // Servos
    analogWriteFrequency(_leftServoPin, PWM_FREQ);
    analogWriteResolution(_leftServoPin, PWM_RES);
    analogWriteFrequency(_rightServoPin, PWM_FREQ);
    analogWriteResolution(_rightServoPin, PWM_RES);

    // Other digital outputs
    pinMode(_extraPin, OUTPUT);
    pinMode(_ledPin, OUTPUT);

    // GPIO 25 is a limit switch input with pullup
    pinMode(_mosfetPin, INPUT_PULLUP);

    // Inputs with pullups
    pinMode(_upperSwPin, INPUT_PULLUP);
    pinMode(_lowerSwPin, INPUT_PULLUP);

    Serial.printf("[PWM] Initialized LeftServo=%d, RightServo=%d, MosfetSwitchPin=%d, Extra=%d, LED=%d, UpperSw=%d, LowerSw=%d\n",
                  _leftServoPin, _rightServoPin, _mosfetPin, _extraPin, _ledPin, _upperSwPin, _lowerSwPin);

    // Startup test sequence: sweep servos synchronously and toggle outputs to verify hardware
    Serial.println("[PWM] Running synchronized boot-up diagnostics sweep...");

    // Sweep: Neutral -> Active -> Neutral
    updateServos(false, minUs, maxUs, invertLeft, invertRight); // Inactive / Neutral
    delay(500);
    updateServos(true, minUs, maxUs, invertLeft, invertRight);  // Active / UP
    delay(500);
    updateServos(false, minUs, maxUs, invertLeft, invertRight); // Inactive / Neutral
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
    // Convert microseconds to duty cycle (12-bit, 50Hz)
    uint32_t duty = (us * 4095) / 20000;
    analogWrite(pin, duty);
}
