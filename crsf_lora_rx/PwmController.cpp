#include "PwmController.h"

#define PWM_FREQ 50
#define PWM_RES 12

PwmController::PwmController(uint8_t leftServoPin, uint8_t rightServoPin, uint8_t mosfetPin, uint8_t extraPin, uint8_t ledPin, uint8_t upperSwPin, uint8_t lowerSwPin)
    : _leftServoPin(leftServoPin), _rightServoPin(rightServoPin), _mosfetPin(mosfetPin), _extraPin(extraPin), _ledPin(ledPin), _upperSwPin(upperSwPin), _lowerSwPin(lowerSwPin) {}

void PwmController::begin() {
    // Servos
    analogWriteFrequency(_leftServoPin, PWM_FREQ);
    analogWriteResolution(_leftServoPin, PWM_RES);
    analogWriteFrequency(_rightServoPin, PWM_FREQ);
    analogWriteResolution(_rightServoPin, PWM_RES);

    // Other digital outputs
    pinMode(_mosfetPin, OUTPUT);
    pinMode(_extraPin, OUTPUT);
    pinMode(_ledPin, OUTPUT);

    // Inputs with pullups
    pinMode(_upperSwPin, INPUT_PULLUP);
    pinMode(_lowerSwPin, INPUT_PULLUP);

    Serial.printf("[PWM] Initialized LeftServo=%d, RightServo=%d, Mosfet=%d, Extra=%d, LED=%d, UpperSw=%d, LowerSw=%d\n",
                  _leftServoPin, _rightServoPin, _mosfetPin, _extraPin, _ledPin, _upperSwPin, _lowerSwPin);

    // Startup test sequence: sweep servos and toggle outputs to verify hardware
    Serial.println("[PWM] Running boot-up diagnostics sweep...");

    // Servo Sweep (Neutral -> Min -> Max -> Neutral)
    writeMicros(_leftServoPin, 1500);
    writeMicros(_rightServoPin, 1500);
    delay(400);
    writeMicros(_leftServoPin, 1000);
    writeMicros(_rightServoPin, 1000);
    delay(400);
    writeMicros(_leftServoPin, 2000);
    writeMicros(_rightServoPin, 2000);
    delay(400);
    writeMicros(_leftServoPin, 1500);
    writeMicros(_rightServoPin, 1500);

    // Toggle MOSFET & Extra & LED
    digitalWrite(_mosfetPin, HIGH);
    digitalWrite(_extraPin, HIGH);
    digitalWrite(_ledPin, HIGH);
    delay(300);
    digitalWrite(_mosfetPin, LOW);
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

void PwmController::updateServos(bool isActive, uint16_t minUs, uint16_t maxUs, bool invertRight) {
    uint32_t leftVal;
    uint32_t rightVal;

    if (isActive) {
        leftVal = maxUs;
        if (invertRight) {
            rightVal = minUs; // Inverted
        } else {
            rightVal = maxUs; // Synchronous
        }
    } else {
        // Neutral/Zero position
        leftVal = (minUs + maxUs) / 2;
        rightVal = (minUs + maxUs) / 2;
    }

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

    digitalWrite(_mosfetPin, outputState);
    digitalWrite(_extraPin, outputState);
}

void PwmController::writeMicros(uint8_t pin, uint32_t us) {
    // Convert microseconds to duty cycle (12-bit, 50Hz)
    uint32_t duty = (us * 4095) / 20000;
    analogWrite(pin, duty);
}
