#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigManager.h"

class WebServerHandler {
public:
    WebServerHandler(ConfigManager &configManager, uint32_t &byteCount, uint32_t &packetCount, uint32_t &loraSentCount);
    void begin();
    void handleClient();

private:
    ConfigManager &_configManager;
    uint32_t &_byteCount;
    uint32_t &_packetCount;
    uint32_t &_loraSentCount;
    WebServer _server;

    void _handleRoot();
    void _handleSave();
    void _handleStatus();
};

#endif
