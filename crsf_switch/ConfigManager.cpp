#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.switchChannel = 15; // Default to Ch 15
    _currentConfig.servoChannel = 16;  // Default to Ch 16
    _currentConfig.cameraChannel = 14; // Default to Ch 14
    _currentConfig.crsfBaudrate = 400000; // Default to 400k

    // Defaults for PWM Limits
    _currentConfig.switchMin = 1000;
    _currentConfig.switchMax = 2000;
    _currentConfig.servoMin = 1000;
    _currentConfig.servoMax = 2000;
    _currentConfig.cameraMin = 1000;
    _currentConfig.cameraMax = 2000;
}

void ConfigManager::begin() {
    _prefs.begin("crsf_config", false);
    _currentConfig.switchChannel = _prefs.getUChar("sw_chan", 15);
    _currentConfig.servoChannel = _prefs.getUChar("srv_chan", 16);
    _currentConfig.cameraChannel = _prefs.getUChar("cam_chan", 14);
    _currentConfig.crsfBaudrate = _prefs.getUInt("crsf_baud", 400000);

    _currentConfig.switchMin = _prefs.getUShort("sw_min", 1000);
    _currentConfig.switchMax = _prefs.getUShort("sw_max", 2000);
    _currentConfig.servoMin = _prefs.getUShort("srv_min", 1000);
    _currentConfig.servoMax = _prefs.getUShort("srv_max", 2000);
    _currentConfig.cameraMin = _prefs.getUShort("cam_min", 1000);
    _currentConfig.cameraMax = _prefs.getUShort("cam_max", 2000);

    // Bounds check mappings
    if (_currentConfig.switchChannel < 1 || _currentConfig.switchChannel > 16) _currentConfig.switchChannel = 15;
    if (_currentConfig.servoChannel < 1 || _currentConfig.servoChannel > 16) _currentConfig.servoChannel = 16;
    if (_currentConfig.cameraChannel < 1 || _currentConfig.cameraChannel > 16) _currentConfig.cameraChannel = 14;

    if (_currentConfig.crsfBaudrate != 115200 &&
        _currentConfig.crsfBaudrate != 400000 &&
        _currentConfig.crsfBaudrate != 420000) {
        _currentConfig.crsfBaudrate = 400000;
    }

    // Bounds check custom PWM limits (500us - 2500us)
    if (_currentConfig.switchMin < 500 || _currentConfig.switchMin > 2500) _currentConfig.switchMin = 1000;
    if (_currentConfig.switchMax < 500 || _currentConfig.switchMax > 2500) _currentConfig.switchMax = 2000;
    if (_currentConfig.servoMin < 500 || _currentConfig.servoMin > 2500) _currentConfig.servoMin = 1000;
    if (_currentConfig.servoMax < 500 || _currentConfig.servoMax > 2500) _currentConfig.servoMax = 2000;
    if (_currentConfig.cameraMin < 500 || _currentConfig.cameraMin > 2500) _currentConfig.cameraMin = 1000;
    if (_currentConfig.cameraMax < 500 || _currentConfig.cameraMax > 2500) _currentConfig.cameraMax = 2000;

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

    _prefs.putUShort("sw_min", _currentConfig.switchMin);
    _prefs.putUShort("sw_max", _currentConfig.switchMax);
    _prefs.putUShort("srv_min", _currentConfig.servoMin);
    _prefs.putUShort("srv_max", _currentConfig.servoMax);
    _prefs.putUShort("cam_min", _currentConfig.cameraMin);
    _prefs.putUShort("cam_max", _currentConfig.cameraMax);
    _prefs.end();

    Serial.printf("Config saved: Switch=Ch%d (%u-%uus), Servo=Ch%d (%u-%uus), Camera=Ch%d (%u-%uus), Baud=%u\n",
                  _currentConfig.switchChannel, _currentConfig.switchMin, _currentConfig.switchMax,
                  _currentConfig.servoChannel, _currentConfig.servoMin, _currentConfig.servoMax,
                  _currentConfig.cameraChannel, _currentConfig.cameraMin, _currentConfig.cameraMax,
                  _currentConfig.crsfBaudrate);
}
