#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigManager.h"

class WebServerHandler {
public:
    WebServerHandler(ConfigManager &configManager, uint32_t &packetCount, uint16_t *channels, bool &upperSw, bool &lowerSw, bool &overrideActive);
    void begin();
    void handleClient();

private:
    ConfigManager &_configManager;
    uint32_t &_packetCount;
    uint16_t *_channels;
    bool &_upperSw;
    bool &_lowerSw;
    bool &_overrideActive;
    WebServer _server;

    void _handleRoot();
    void _handleSave();
    void _handleStatus();
};

#endif
