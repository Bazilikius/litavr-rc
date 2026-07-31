#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.servoChannel = 16;
    _currentConfig.servoTrigger = 1500;
    _currentConfig.servoInvertLeft = 0;
    _currentConfig.servoInvertRight = 0;
    _currentConfig.servoMin = 1000;
    _currentConfig.servoMax = 2000;
    _currentConfig.loraFreq = 433000000;
}

void ConfigManager::begin() {
    _prefs.begin("crsf_rx_config", false);
    _currentConfig.servoChannel = _prefs.getUChar("srv_chan", 16);
    _currentConfig.servoTrigger = _prefs.getUShort("srv_trig", 1500);
    _currentConfig.servoInvertLeft = _prefs.getUChar("srv_inv_l", 0);
    _currentConfig.servoInvertRight = _prefs.getUChar("srv_inv", 0);
    _currentConfig.servoMin = _prefs.getUShort("srv_min", 1000);
    _currentConfig.servoMax = _prefs.getUShort("srv_max", 2000);
    _currentConfig.loraFreq = _prefs.getUInt("lora_freq", 433000000);

    // Validate ranges
    if (_currentConfig.servoChannel < 1 || _currentConfig.servoChannel > 16) _currentConfig.servoChannel = 16;
    if (_currentConfig.servoMin < 500 || _currentConfig.servoMin > 2500) _currentConfig.servoMin = 1000;
    if (_currentConfig.servoMax < 500 || _currentConfig.servoMax > 2500) _currentConfig.servoMax = 2000;

    if (_currentConfig.loraFreq < 100000000 || _currentConfig.loraFreq > 1000000000) {
        _currentConfig.loraFreq = 433000000;
    }

    _prefs.end();
}

RxConfig ConfigManager::getConfig() {
    return _currentConfig;
}

void ConfigManager::saveConfig(RxConfig config) {
    _currentConfig = config;
    _prefs.begin("crsf_rx_config", false);
    _prefs.putUChar("srv_chan", _currentConfig.servoChannel);
    _prefs.putUShort("srv_trig", _currentConfig.servoTrigger);
    _prefs.putUChar("srv_inv_l", _currentConfig.servoInvertLeft);
    _prefs.putUChar("srv_inv", _currentConfig.servoInvertRight);
    _prefs.putUShort("srv_min", _currentConfig.servoMin);
    _prefs.putUShort("srv_max", _currentConfig.servoMax);
    _prefs.putUInt("lora_freq", _currentConfig.loraFreq);
    _prefs.end();

    Serial.printf("RX Config saved: Servo=Ch%d (Trig:%u, Min:%u, Max:%u, InvL:%d, InvR:%d) | LoraFreq:%u\n",
                  _currentConfig.servoChannel, _currentConfig.servoTrigger, _currentConfig.servoMin, _currentConfig.servoMax,
                  _currentConfig.servoInvertLeft, _currentConfig.servoInvertRight, _currentConfig.loraFreq);
}
