#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.servoChannel = 16;
    _currentConfig.servoTrigger = 1500;
    _currentConfig.servoInvertRight = 0;
    _currentConfig.servoMin = 1000;
    _currentConfig.servoMax = 2000;

    _currentConfig.mosfetChannel = 15;
    _currentConfig.mosfetTrigger = 1000;
    _currentConfig.mosfetOffLevel = 0; // LOW is OFF

    _currentConfig.allOneChannel = 0;
    _currentConfig.loraFreq = 433000000;
}

void ConfigManager::begin() {
    _prefs.begin("crsf_rx_config", false);
    _currentConfig.servoChannel = _prefs.getUChar("srv_chan", 16);
    _currentConfig.servoTrigger = _prefs.getUShort("srv_trig", 1500);
    _currentConfig.servoInvertRight = _prefs.getUChar("srv_inv", 0);
    _currentConfig.servoMin = _prefs.getUShort("srv_min", 1000);
    _currentConfig.servoMax = _prefs.getUShort("srv_max", 2000);

    _currentConfig.mosfetChannel = _prefs.getUChar("mos_chan", 15);
    _currentConfig.mosfetTrigger = _prefs.getUShort("mos_trig", 1000);
    _currentConfig.mosfetOffLevel = _prefs.getUChar("mos_off", 0);

    _currentConfig.allOneChannel = _prefs.getUChar("all_one", 0);
    _currentConfig.loraFreq = _prefs.getUInt("lora_freq", 433000000);

    // Validate ranges
    if (_currentConfig.servoChannel < 1 || _currentConfig.servoChannel > 16) _currentConfig.servoChannel = 16;
    if (_currentConfig.mosfetChannel < 1 || _currentConfig.mosfetChannel > 16) _currentConfig.mosfetChannel = 15;

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

    // Enforce "All One Channel" if active
    if (_currentConfig.allOneChannel) {
        _currentConfig.mosfetChannel = _currentConfig.servoChannel;
        _currentConfig.mosfetTrigger = _currentConfig.servoTrigger;
    }

    _prefs.begin("crsf_rx_config", false);
    _prefs.putUChar("srv_chan", _currentConfig.servoChannel);
    _prefs.putUShort("srv_trig", _currentConfig.servoTrigger);
    _prefs.putUChar("srv_inv", _currentConfig.servoInvertRight);
    _prefs.putUShort("srv_min", _currentConfig.servoMin);
    _prefs.putUShort("srv_max", _currentConfig.servoMax);

    _prefs.putUChar("mos_chan", _currentConfig.mosfetChannel);
    _prefs.putUShort("mos_trig", _currentConfig.mosfetTrigger);
    _prefs.putUChar("mos_off", _currentConfig.mosfetOffLevel);

    _prefs.putUChar("all_one", _currentConfig.allOneChannel);
    _prefs.putUInt("lora_freq", _currentConfig.loraFreq);
    _prefs.end();

    Serial.printf("RX Config saved: Servo=Ch%d (Trig:%u, Min:%u, Max:%u, Inv:%d) | MOSFET=Ch%d (Trig:%u, OffLevel:%d) | AllOne:%d | LoraFreq:%u\n",
                  _currentConfig.servoChannel, _currentConfig.servoTrigger, _currentConfig.servoMin, _currentConfig.servoMax, _currentConfig.servoInvertRight,
                  _currentConfig.mosfetChannel, _currentConfig.mosfetTrigger, _currentConfig.mosfetOffLevel,
                  _currentConfig.allOneChannel, _currentConfig.loraFreq);
}
