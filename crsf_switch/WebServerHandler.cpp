#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(ConfigManager &configManager, CrsfParser &parser, uint32_t &byteCount, uint32_t &packetCount)
    : _configManager(configManager), _parser(parser), _byteCount(byteCount), _packetCount(packetCount), _server(80) {}

void WebServerHandler::begin() {
    // Start AP mode
    WiFi.softAP("CRSF-Config", "12345678");
    Serial.println("WiFi Access Point Started: SSID='CRSF-Config', Password='12345678'");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Setup routes
    _server.on("/", std::bind(&WebServerHandler::_handleRoot, this));
    _server.on("/save", std::bind(&WebServerHandler::_handleSave, this));
    _server.on("/status", std::bind(&WebServerHandler::_handleStatus, this));

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
    html += "<title>CRSF Outputs & Web Monitor</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; text-align: center; padding: 20px; }";
    html += ".container { background-color: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; max-width: 500px; width: 100%; text-align: left; }";
    html += "h1 { color: #007bff; margin-bottom: 20px; font-size: 24px; text-align: center; }";
    html += "label { display: block; margin: 10px 0 5px; font-weight: bold; }";
    html += "select { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 15px; background: #fff; }";
    html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; font-weight: bold; }";
    html += "input[type='submit']:hover { background-color: #0056b3; }";
    html += ".panel { background: #eef2f7; border: 1px solid #ddd; padding: 15px; border-radius: 6px; margin-bottom: 20px; }";
    html += ".panel h3 { margin-top: 0; color: #333; }";
    html += ".panel p { margin: 5px 0; font-family: monospace; font-size: 14px; }";
    html += ".status-indicator { font-weight: bold; color: red; }";
    html += ".status-ok { color: green; }";
    html += ".grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 5px; font-family: monospace; font-size: 12px; margin-top: 10px; }";
    html += ".grid-item { background: #ddd; padding: 5px; text-align: center; border-radius: 3px; }";
    html += "</style>";
    html += "<script>";
    html += "function updateStatus() {";
    html += "  fetch('/status').then(r => r.json()).then(data => {";
    html += "    document.getElementById('byteCount').innerText = data.bytes;";
    html += "    document.getElementById('packetCount').innerText = data.packets;";
    html += "    const link = document.getElementById('linkStatus');";
    html += "    if(data.bytes > 0 && data.packets > 0) {";
    html += "      link.innerText = 'ONLINE'; link.className = 'status-indicator status-ok';";
    html += "    } else {";
    html += "      link.innerText = 'NO DATA'; link.className = 'status-indicator';";
    html += "    }";
    html += "    for(let i=0; i<16; i++) {";
    html += "      const cell = document.getElementById('ch' + i);";
    html += "      if(cell) cell.innerText = 'CH' + (i+1) + ': ' + data.channels[i];";
    html += "    }";
    html += "  });";
    html += "}";
    html += "setInterval(updateStatus, 1000);";
    html += "</script>";
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<h1>CRSF Web Config & Port Monitor</h1>";

    // Live Serial Status Panel
    html += "<div class='panel'>";
    html += "<h3>Live Link Monitor</h3>";
    html += "<p>Link Status: <span id='linkStatus' class='status-indicator'>CONNECTING...</span></p>";
    html += "<p>Bytes Received: <span id='byteCount'>0</span></p>";
    html += "<p>Parsed Packets: <span id='packetCount'>0</span></p>";
    html += "<strong>Channel Values (ticks):</strong>";
    html += "<div class='grid'>";
    for (int i = 0; i < 16; i++) {
        html += "<div class='grid-item' id='ch" + String(i) + "'>CH" + String(i+1) + ": -</div>";
    }
    html += "</div>";
    html += "</div>";

    // Configuration Form
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

    // Camera Switch Channel Dropdown
    html += "<label for='camera'>Camera Switch Output (GPIO 3) -> CRSF Channel:</label>";
    html += "<select name='camera' id='camera'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.cameraChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    // CRSF Baudrate Dropdown
    html += "<label for='baud'>CRSF Receiver Baudrate:</label>";
    html += "<select name='baud' id='baud'>";
    uint32_t bauds[] = {115200, 400000, 420000};
    for (int i = 0; i < 3; i++) {
        String selected = (config.crsfBaudrate == bauds[i]) ? "selected" : "";
        html += "<option value='" + String(bauds[i]) + "' " + selected + ">" + String(bauds[i]) + "</option>";
    }
    html += "</select>";

    html += "<input type='submit' value='Save & Apply Configuration'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";

    _server.send(200, "text/html", html);
}

void WebServerHandler::_handleSave() {
    if (_server.hasArg("switch") && _server.hasArg("servo") && _server.hasArg("camera") && _server.hasArg("baud")) {
        OutputConfig newConfig;
        newConfig.switchChannel = _server.arg("switch").toInt();
        newConfig.servoChannel = _server.arg("servo").toInt();
        newConfig.cameraChannel = _server.arg("camera").toInt();
        newConfig.crsfBaudrate = _server.arg("baud").toInt();

        if (newConfig.switchChannel >= 1 && newConfig.switchChannel <= 16 &&
            newConfig.servoChannel >= 1 && newConfig.servoChannel <= 16 &&
            newConfig.cameraChannel >= 1 && newConfig.cameraChannel <= 16 &&
            (newConfig.crsfBaudrate == 115200 || newConfig.crsfBaudrate == 400000 || newConfig.crsfBaudrate == 420000)) {

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
            response += "<p>Camera Switch mapped to CH " + String(newConfig.cameraChannel) + "</p>";
            response += "<p>Baudrate set to " + String(newConfig.crsfBaudrate) + "</p>";
            response += "<p><strong>Note: Re-applying baudrate configuration requires restarting the device.</strong></p>";
            response += "<a href='/'>&larr; Back to configuration</a>";
            response += "</div></body></html>";

            _server.send(200, "text/html", response);

            // Allow the response to send, then reboot the device to apply baudrate changes safely
            delay(1000);
            ESP.restart();
            return;
        }
    }
    _server.send(400, "text/plain", "Invalid Parameters.");
}

void WebServerHandler::_handleStatus() {
    String json = "{";
    json += "\"bytes\":" + String(_byteCount) + ",";
    json += "\"packets\":" + String(_packetCount) + ",";
    json += "\"channels\":[";
    for (int i = 0; i < 16; i++) {
        json += String(_parser.getChannel(i));
        if (i < 15) json += ",";
    }
    json += "]}";
    _server.send(200, "application/json", json);
}
