#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

struct TxConfig {
    uint32_t crsfBaudrate; // 115200, 400000, 420000
    uint32_t loraFreq;     // e.g. 433000000, 868000000
};

class ConfigManager {
public:
    ConfigManager();
    void begin();
    TxConfig getConfig();
    void saveConfig(TxConfig config);

private:
    Preferences _prefs;
    TxConfig _currentConfig;
};

#endif
