#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct RxConfig {
    uint8_t servoChannel;      // 1-16, default 16
    uint16_t servoTrigger;     // 1000, 1500, 2000, default 1500
    uint8_t servoInvertRight;  // 0 or 1, default 0
    uint16_t servoMin;         // 500-2500 us, default 1000
    uint16_t servoMax;         // 500-2500 us, default 2000

    uint8_t mosfetChannel;     // 1-16, default 15
    uint16_t mosfetTrigger;    // 1000, 1500, 2000, default 1000
    uint8_t mosfetOffLevel;    // 0 = LOW, 1 = HIGH, default 0

    uint8_t allOneChannel;     // 0 or 1, default 0
    uint32_t loraFreq;         // e.g. 433000000
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
