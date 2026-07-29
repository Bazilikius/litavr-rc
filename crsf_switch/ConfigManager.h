#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct OutputConfig {
    uint8_t switchChannel; // 1-16
    uint8_t servoChannel;  // 1-16
    uint8_t cameraChannel; // 1-16
    uint32_t crsfBaudrate; // 115200, 400000, 420000
};

class ConfigManager {
public:
    ConfigManager();
    void begin();
    OutputConfig getConfig();
    void saveConfig(OutputConfig config);

private:
    Preferences _prefs;
    OutputConfig _currentConfig;
};

#endif
