#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.switchChannel = 15; // Default to Ch 15
    _currentConfig.servoChannel = 16;  // Default to Ch 16
    _currentConfig.cameraChannel = 14; // Default to Ch 14
    _currentConfig.crsfBaudrate = 400000; // Default to 400k
}

void ConfigManager::begin() {
    _prefs.begin("crsf_config", false);
    _currentConfig.switchChannel = _prefs.getUChar("sw_chan", 15);
    _currentConfig.servoChannel = _prefs.getUChar("srv_chan", 16);
    _currentConfig.cameraChannel = _prefs.getUChar("cam_chan", 14);
    _currentConfig.crsfBaudrate = _prefs.getUInt("crsf_baud", 400000);

    // Bounds check
    if (_currentConfig.switchChannel < 1 || _currentConfig.switchChannel > 16) {
        _currentConfig.switchChannel = 15;
    }
    if (_currentConfig.servoChannel < 1 || _currentConfig.servoChannel > 16) {
        _currentConfig.servoChannel = 16;
    }
    if (_currentConfig.cameraChannel < 1 || _currentConfig.cameraChannel > 16) {
        _currentConfig.cameraChannel = 14;
    }
    if (_currentConfig.crsfBaudrate != 115200 &&
        _currentConfig.crsfBaudrate != 400000 &&
        _currentConfig.crsfBaudrate != 420000) {
        _currentConfig.crsfBaudrate = 400000;
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
    _prefs.putUChar("cam_chan", _currentConfig.cameraChannel);
    _prefs.putUInt("crsf_baud", _currentConfig.crsfBaudrate);
    _prefs.end();
    Serial.printf("Config saved: Switch=Ch%d, Servo=Ch%d, Camera=Ch%d, Baudrate=%d\n",
                  _currentConfig.switchChannel, _currentConfig.servoChannel,
                  _currentConfig.cameraChannel, _currentConfig.crsfBaudrate);
}
