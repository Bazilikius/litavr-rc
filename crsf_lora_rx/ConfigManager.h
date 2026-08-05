#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct RxConfig {
    uint8_t boxId;             // 1-8, default 1
    uint8_t boxSelectChannel;  // 1-16, default 15
    uint8_t servoChannel;      // 1-16, default 16
    uint16_t servoTrigger;     // 1000, 1500, 2000, default 1500
    uint8_t servoInvertLeft;   // 0 or 1, default 0
    uint8_t servoInvertRight;  // 0 or 1, default 0
    uint16_t servoMin;         // 500-2500 us, default 1500
    uint16_t servoMax;         // 500-2500 us, default 2000
    uint16_t servoSpeed;       // us per second, default 100
    uint32_t loraFreq;         // e.g. 433000000

    uint8_t allOneChannel;     // 0 or 1, default 0
    uint8_t mosfetOffLevel;    // 0 = LOW, 1 = HIGH, default 0
    uint8_t powerKeyOffLevel;  // 0 = LOW, 1 = HIGH, default 0
    uint8_t useThreeSwitches;  // 0 = 2 Switches, 1 = 3 Switches, default 0
    uint16_t mosfetActiveWidth; // 2000 or 2500, default 2000
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
