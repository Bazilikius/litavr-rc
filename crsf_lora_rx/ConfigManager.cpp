#include "ConfigManager.h"

ConfigManager::ConfigManager() {
    _currentConfig.boxId = 1;
    _currentConfig.boxSelectChannel = 15;
    _currentConfig.servoChannel = 16;
    _currentConfig.servoTrigger = 1500;
    _currentConfig.servoInvertLeft = 0;
    _currentConfig.servoInvertRight = 0;
    _currentConfig.servoMin = 1500; // Default: 1500us
    _currentConfig.servoMax = 2000; // Default: 2000us
    _currentConfig.loraFreq = 433000000;
    _currentConfig.allOneChannel = 0;
    _currentConfig.mosfetOffLevel = 0;
}

void ConfigManager::begin() {
    _prefs.begin("crsf_rx_config2", false);
    _currentConfig.boxId = _prefs.getUChar("box_id", 1);
    _currentConfig.boxSelectChannel = _prefs.getUChar("box_sel_ch", 15);
    _currentConfig.servoChannel = _prefs.getUChar("srv_chan", 16);
    _currentConfig.servoTrigger = _prefs.getUShort("srv_trig", 1500);
    _currentConfig.servoInvertLeft = _prefs.getUChar("srv_inv_l", 0);
    _currentConfig.servoInvertRight = _prefs.getUChar("srv_inv", 0);
    _currentConfig.servoMin = _prefs.getUShort("srv_min", 1500);
    _currentConfig.servoMax = _prefs.getUShort("srv_max", 2000);
    _currentConfig.loraFreq = _prefs.getUInt("lora_freq", 433000000);
    _currentConfig.allOneChannel = _prefs.getUChar("all_one", 0);
    _currentConfig.mosfetOffLevel = _prefs.getUChar("mos_off", 0);

    // Validate ranges
    if (_currentConfig.boxId < 1 || _currentConfig.boxId > 8) _currentConfig.boxId = 1;
    if (_currentConfig.boxSelectChannel < 1 || _currentConfig.boxSelectChannel > 16) _currentConfig.boxSelectChannel = 15;
    if (_currentConfig.servoChannel < 1 || _currentConfig.servoChannel > 16) _currentConfig.servoChannel = 16;
    if (_currentConfig.servoMin < 500 || _currentConfig.servoMin > 2500) _currentConfig.servoMin = 1500;
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

    // Duplication for "All one channel" mode (if selected, box selector is also mapped to the servo channel)
    if (_currentConfig.allOneChannel) {
        _currentConfig.boxSelectChannel = _currentConfig.servoChannel;
    }

    _prefs.begin("crsf_rx_config2", false);
    _prefs.putUChar("box_id", _currentConfig.boxId);
    _prefs.putUChar("box_sel_ch", _currentConfig.boxSelectChannel);
    _prefs.putUChar("srv_chan", _currentConfig.servoChannel);
    _prefs.putUShort("srv_trig", _currentConfig.servoTrigger);
    _prefs.putUChar("srv_inv_l", _currentConfig.servoInvertLeft);
    _prefs.putUChar("srv_inv", _currentConfig.servoInvertRight);
    _prefs.putUShort("srv_min", _currentConfig.servoMin);
    _prefs.putUShort("srv_max", _currentConfig.servoMax);
    _prefs.putUInt("lora_freq", _currentConfig.loraFreq);
    _prefs.putUChar("all_one", _currentConfig.allOneChannel);
    _prefs.putUChar("mos_off", _currentConfig.mosfetOffLevel);
    _prefs.end();

    Serial.printf("RX Config saved: BoxId:%d | BoxSelectCh:Ch%d | Servo=Ch%d (Trig:%u, Min:%u, Max:%u, InvL:%d, InvR:%d) | AllOne:%d | MosfetOff:%d | LoraFreq:%u\n",
                  _currentConfig.boxId, _currentConfig.boxSelectChannel,
                  _currentConfig.servoChannel, _currentConfig.servoTrigger,
                  _currentConfig.servoMin, _currentConfig.servoMax,
                  _currentConfig.servoInvertLeft, _currentConfig.servoInvertRight,
                  _currentConfig.allOneChannel, _currentConfig.mosfetOffLevel, _currentConfig.loraFreq);
}
