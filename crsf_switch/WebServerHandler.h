#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigManager.h"
#include "CrsfParser.h"

class WebServerHandler {
public:
    WebServerHandler(ConfigManager &configManager, CrsfParser &parser, uint32_t &byteCount, uint32_t &packetCount);
    void begin();
    void handleClient();

private:
    ConfigManager &_configManager;
    CrsfParser &_parser;
    uint32_t &_byteCount;
    uint32_t &_packetCount;
    WebServer _server;

    void _handleRoot();
    void _handleSave();
    void _handleStatus();
};

#endif
