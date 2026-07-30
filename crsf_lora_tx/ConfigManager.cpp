#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.crsfBaudrate = 400000; // Default to 400k
    _currentConfig.loraFreq = 433000000;  // Default to 433MHz
}

void ConfigManager::begin() {
    _prefs.begin("crsf_tx_config", false);
    _currentConfig.crsfBaudrate = _prefs.getUInt("crsf_baud", 400000);
    _currentConfig.loraFreq = _prefs.getUInt("lora_freq", 433000000);

    if (_currentConfig.crsfBaudrate != 115200 &&
        _currentConfig.crsfBaudrate != 400000 &&
        _currentConfig.crsfBaudrate != 420000) {
        _currentConfig.crsfBaudrate = 400000;
    }

    if (_currentConfig.loraFreq < 100000000 || _currentConfig.loraFreq > 1000000000) {
        _currentConfig.loraFreq = 433000000;
    }

    _prefs.end();
}

TxConfig ConfigManager::getConfig() {
    return _currentConfig;
}

void ConfigManager::saveConfig(TxConfig config) {
    _currentConfig = config;
    _prefs.begin("crsf_tx_config", false);
    _prefs.putUInt("crsf_baud", _currentConfig.crsfBaudrate);
    _prefs.putUInt("lora_freq", _currentConfig.loraFreq);
    _prefs.end();

    Serial.printf("TX Config saved: Baud=%u, LoRa Freq=%u\n",
                  _currentConfig.crsfBaudrate, _currentConfig.loraFreq);
}
