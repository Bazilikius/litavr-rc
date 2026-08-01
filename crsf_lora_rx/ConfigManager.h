#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct RxConfig {
    uint8_t servoChannel;      // 1-16, default 16
    uint16_t servoTrigger;     // 1000, 1500, 2000, default 1500
    uint8_t servoInvertLeft;   // 0 or 1, default 0
    uint8_t servoInvertRight;  // 0 or 1, default 0
    uint16_t servoMin;         // 500-2500 us, default 1000
    uint16_t servoMax;         // 500-2500 us, default 2000
    uint32_t loraFreq;         // e.g. 433000000

    // Switch active/trigger polarities: 0 = Active LOW (Closed), 1 = Active HIGH (Open)
    uint8_t upperSwPolarity;   // default 1 (Open)
    uint8_t lowerSwPolarity;   // default 1 (Open)
    uint8_t servoUpSwPolarity; // default 1 (Open)

    // Output MOSFET polarities: 0 = Active HIGH / Inactive LOW, 1 = Active LOW / Inactive HIGH
    uint8_t powerKeyOffLevel;  // default 0 (LOW = Inactive / OFF)
    uint8_t extraPinOffLevel;  // default 0 (LOW = Inactive / OFF)
};

class ConfigManager {
public:
    ConfigManager();
    void begin();
    RxConfig getConfig();
    void saveConfig(RxConfig config);

private:
    Preferences _prefs;
    RxConfig _currentConfig;
};

#endif
