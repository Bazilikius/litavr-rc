#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.switchChannel = 15; // Default to Ch 15
    _currentConfig.servoChannel = 16;  // Default to Ch 16
}

void ConfigManager::begin() {
    _prefs.begin("crsf_config", false);
    _currentConfig.switchChannel = _prefs.getUChar("sw_chan", 15);
    _currentConfig.servoChannel = _prefs.getUChar("srv_chan", 16);

    // Bounds check
    if (_currentConfig.switchChannel < 1 || _currentConfig.switchChannel > 16) {
        _currentConfig.switchChannel = 15;
    }
    if (_currentConfig.servoChannel < 1 || _currentConfig.servoChannel > 16) {
        _currentConfig.servoChannel = 16;
    }
    _prefs.end();
}

OutputConfig ConfigManager::getConfig() {
    return _currentConfig;
}

void ConfigManager::saveConfig(OutputConfig config) {
    _currentConfig = config;
    _prefs.begin("crsf_config", false);
    _prefs.putUChar("sw_chan", _currentConfig.switchChannel);
    _prefs.putUChar("srv_chan", _currentConfig.servoChannel);
    _prefs.end();
    Serial.printf("Config saved: Switch=Ch%d, Servo=Ch%d\n", _currentConfig.switchChannel, _currentConfig.servoChannel);
}
