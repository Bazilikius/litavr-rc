#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(ConfigManager &configManager)
    : _configManager(configManager), _server(80) {}

void WebServerHandler::begin() {
    // Set up ESP32 AP Mode
    WiFi.softAP("CRSF-Config", "12345678");
    Serial.println("WiFi Access Point Started: SSID='CRSF-Config', Password='12345678'");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Setup routes
    _server.on("/", std::bind(&WebServerHandler::_handleRoot, this));
    _server.on("/save", std::bind(&WebServerHandler::_handleSave, this));

    _server.begin();
    Serial.println("HTTP Web Server Started.");
}

void WebServerHandler::handleClient() {
    _server.handleClient();
}

void WebServerHandler::_handleRoot() {
    OutputConfig config = _configManager.getConfig();

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>CRSF Output Configurator</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; text-align: center; padding: 20px; }";
    html += ".container { background-color: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; max-width: 400px; width: 100%; }";
    html += "h1 { color: #007bff; margin-bottom: 20px; font-size: 24px; }";
    html += "label { display: block; text-align: left; margin: 10px 0 5px; font-weight: bold; }";
    html += "select { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 20px; }";
    html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; }";
    html += "input[type='submit']:hover { background-color: #0056b3; }";
    html += ".footer { margin-top: 20px; font-size: 12px; color: #777; }";
    html += "</style></head><body>";

    html += "<div class='container'>";
    html += "<h1>CRSF Output Mapping</h1>";
    html += "<form action='/save' method='POST'>";

    // Switch Channel Dropdown
    html += "<label for='switch'>Switch Output (GPIO 5) -> CRSF Channel:</label>";
    html += "<select name='switch' id='switch'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.switchChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    // Servo Channel Dropdown
    html += "<label for='servo'>Servo Output (GPIO 4) -> CRSF Channel:</label>";
    html += "<select name='servo' id='servo'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.servoChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<input type='submit' value='Save Configuration'>";
    html += "</form>";
    html += "<div class='footer'>ESP32-C3 Super Mini Control</div>";
    html += "</div>";

    html += "</body></html>";

    _server.send(200, "text/html", html);
}

void WebServerHandler::_handleSave() {
    if (_server.hasArg("switch") && _server.hasArg("servo")) {
        OutputConfig newConfig;
        newConfig.switchChannel = _server.arg("switch").toInt();
        newConfig.servoChannel = _server.arg("servo").toInt();

        // Bounds validation
        if (newConfig.switchChannel >= 1 && newConfig.switchChannel <= 16 &&
            newConfig.servoChannel >= 1 && newConfig.servoChannel <= 16) {

            _configManager.saveConfig(newConfig);

            String response = "<!DOCTYPE html><html><head>";
            response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
            response += "<title>Configuration Saved</title>";
            response += "<style>";
            response += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; padding: 50px; }";
            response += ".message { background: #d4edda; color: #155724; padding: 20px; border-radius: 8px; border: 1px solid #c3e6cb; display: inline-block; max-width: 400px; }";
            response += "a { display: inline-block; margin-top: 15px; color: #007bff; text-decoration: none; font-weight: bold; }";
            response += "</style></head><body>";
            response += "<div class='message'>";
            response += "<h2>Configuration Saved Successfully!</h2>";
            response += "<p>Switch mapped to CH " + String(newConfig.switchChannel) + "</p>";
            response += "<p>Servo mapped to CH " + String(newConfig.servoChannel) + "</p>";
            response += "<a href='/'>&larr; Back to configuration</a>";
            response += "</div></body></html>";

            _server.send(200, "text/html", response);
            return;
        }
    }
    _server.send(400, "text/plain", "Invalid Parameters.");
}
