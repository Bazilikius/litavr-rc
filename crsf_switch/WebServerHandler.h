#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "ConfigManager.h"

class WebServerHandler {
public:
    WebServerHandler(ConfigManager &configManager);
    void begin();
    void handleClient();

private:
    ConfigManager &_configManager;
    WebServer _server;

    void _handleRoot();
    void _handleSave();
};

#endif
