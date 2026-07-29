#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(ConfigManager &configManager, CrsfParser &parser, uint32_t &byteCount, uint32_t &packetCount)
    : _configManager(configManager), _parser(parser), _byteCount(byteCount), _packetCount(packetCount), _server(80) {}

#include <esp_wifi.h>

void WebServerHandler::begin() {
    // Start AP mode
    WiFi.softAP("CRSF-Config", "12345678");

    // Reduce WiFi Tx Power to 50% (approx 10dBm out of max 20dBm) for stability & low power
    // ESP32-C3 maximum Tx power is 20dBm (corresponding to WIFI_POWER_19_5dBm)
    // 50% power corresponds to ~10dBm (or WIFI_POWER_11dBm/WIFI_POWER_8_5dBm)
    // We can use WiFi.setTxPower(WIFI_POWER_11dBm) or WIFI_POWER_8_5dBm, or set to 10.
    WiFi.setTxPower(WIFI_POWER_11dBm);

    Serial.println("WiFi Access Point Started: SSID='CRSF-Config', Password='12345678'");
    Serial.printf("WiFi Transmit Power limited to 11dBm (~50%% power output).\n");
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
    html += "select, input[type='number'] { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 15px; background: #fff; box-sizing: border-box; }";
    html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; font-weight: bold; }";
    html += "input[type='submit']:hover { background-color: #0056b3; }";
    html += ".panel { background: #eef2f7; border: 1px solid #ddd; padding: 15px; border-radius: 6px; margin-bottom: 20px; }";
    html += ".panel h3 { margin-top: 0; color: #333; }";
    html += ".panel p { margin: 5px 0; font-family: monospace; font-size: 14px; }";
    html += ".status-indicator { font-weight: bold; color: red; }";
    html += ".status-ok { color: green; }";
    html += ".grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 5px; font-family: monospace; font-size: 12px; margin-top: 10px; }";
    html += ".grid-item { background: #ddd; padding: 5px; text-align: center; border-radius: 3px; }";
    html += ".row { display: flex; gap: 10px; }";
    html += ".row div { flex: 1; }";
    html += "hr { border: 0; border-top: 1px solid #ddd; margin: 20px 0; }";
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

    // Switch Channel Dropdown & Limits
    html += "<h3>1. Switch Output (GPIO 5)</h3>";
    html += "<label for='switch'>CRSF Channel Map:</label>";
    html += "<select name='switch' id='switch'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.switchChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<div class='row'>";
    html += "  <div><label>Min PWM (us):</label><input type='number' name='sw_min' value='" + String(config.switchMin) + "' min='500' max='2500'></div>";
    html += "  <div><label>Max PWM (us):</label><input type='number' name='sw_max' value='" + String(config.switchMax) + "' min='500' max='2500'></div>";
    html += "</div>";

    html += "<hr>";

    // Servo Channel Dropdown & Limits
    html += "<h3>2. Servo Output (GPIO 4)</h3>";
    html += "<label for='servo'>CRSF Channel Map:</label>";
    html += "<select name='servo' id='servo'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.servoChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<div class='row'>";
    html += "  <div><label>Min PWM (us):</label><input type='number' name='srv_min' value='" + String(config.servoMin) + "' min='500' max='2500'></div>";
    html += "  <div><label>Max PWM (us):</label><input type='number' name='srv_max' value='" + String(config.servoMax) + "' min='500' max='2500'></div>";
    html += "</div>";

    html += "<hr>";

    // Camera Switch Channel Dropdown & Limits
    html += "<h3>3. Camera Switch Output (GPIO 3)</h3>";
    html += "<label for='camera'>CRSF Channel Map:</label>";
    html += "<select name='camera' id='camera'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.cameraChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<div class='row'>";
    html += "  <div><label>Min PWM (us):</label><input type='number' name='cam_min' value='" + String(config.cameraMin) + "' min='500' max='2500'></div>";
    html += "  <div><label>Max PWM (us):</label><input type='number' name='cam_max' value='" + String(config.cameraMax) + "' min='500' max='2500'></div>";
    html += "</div>";

    html += "<hr>";

    // CRSF Baudrate Dropdown
    html += "<h3>4. System Configuration</h3>";
    html += "<label for='baud'>CRSF Receiver Baudrate:</label>";
    html += "<select name='baud' id='baud'>";
    uint32_t bauds[] = {115200, 400000, 420000};
    for (int i = 0; i < 3; i++) {
        String selected = (config.crsfBaudrate == bauds[i]) ? "selected" : "";
        html += "<option value='" + String(bauds[i]) + "' " + selected + ">" + String(bauds[i]) + " baud</option>";
    }
    html += "</select>";

    html += "<input type='submit' value='Save & Apply Configuration'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";

    _server.send(200, "text/html", html);
}

void WebServerHandler::_handleSave() {
    if (_server.hasArg("switch") && _server.hasArg("servo") && _server.hasArg("camera") && _server.hasArg("baud") &&
        _server.hasArg("sw_min") && _server.hasArg("sw_max") &&
        _server.hasArg("srv_min") && _server.hasArg("srv_max") &&
        _server.hasArg("cam_min") && _server.hasArg("cam_max")) {

        OutputConfig newConfig;
        newConfig.switchChannel = _server.arg("switch").toInt();
        newConfig.servoChannel = _server.arg("servo").toInt();
        newConfig.cameraChannel = _server.arg("camera").toInt();
        newConfig.crsfBaudrate = _server.arg("baud").toInt();

        newConfig.switchMin = _server.arg("sw_min").toInt();
        newConfig.switchMax = _server.arg("sw_max").toInt();
        newConfig.servoMin = _server.arg("srv_min").toInt();
        newConfig.servoMax = _server.arg("srv_max").toInt();
        newConfig.cameraMin = _server.arg("cam_min").toInt();
        newConfig.cameraMax = _server.arg("cam_max").toInt();

        // Validate all ranges and thresholds (500us - 2500us)
        if (newConfig.switchChannel >= 1 && newConfig.switchChannel <= 16 &&
            newConfig.servoChannel >= 1 && newConfig.servoChannel <= 16 &&
            newConfig.cameraChannel >= 1 && newConfig.cameraChannel <= 16 &&
            newConfig.switchMin >= 500 && newConfig.switchMin <= 2500 &&
            newConfig.switchMax >= 500 && newConfig.switchMax <= 2500 &&
            newConfig.servoMin >= 500 && newConfig.servoMin <= 2500 &&
            newConfig.servoMax >= 500 && newConfig.servoMax <= 2500 &&
            newConfig.cameraMin >= 500 && newConfig.cameraMin <= 2500 &&
            newConfig.cameraMax >= 500 && newConfig.cameraMax <= 2500 &&
            (newConfig.crsfBaudrate == 115200 || newConfig.crsfBaudrate == 400000 || newConfig.crsfBaudrate == 420000)) {

            _configManager.saveConfig(newConfig);

            String response = "<!DOCTYPE html><html><head>";
            response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
            response += "<title>Configuration Saved</title>";
            response += "<style>";
            response += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; padding: 50px; }";
            response += ".message { background: #d4edda; color: #155724; padding: 20px; border-radius: 8px; border: 1px solid #c3e6cb; display: inline-block; max-width: 400px; text-align: left; }";
            response += "a { display: inline-block; margin-top: 15px; color: #007bff; text-decoration: none; font-weight: bold; }";
            response += "</style></head><body>";
            response += "<div class='message'>";
            response += "<h2>Configuration Saved Successfully!</h2>";
            response += "<p><b>Switch (GPIO 5):</b> CH" + String(newConfig.switchChannel) + " (" + String(newConfig.switchMin) + "us - " + String(newConfig.switchMax) + "us)</p>";
            response += "<p><b>Servo (GPIO 4):</b> CH" + String(newConfig.servoChannel) + " (" + String(newConfig.servoMin) + "us - " + String(newConfig.servoMax) + "us)</p>";
            response += "<p><b>Camera (GPIO 3):</b> CH" + String(newConfig.cameraChannel) + " (" + String(newConfig.cameraMin) + "us - " + String(newConfig.cameraMax) + "us)</p>";
            response += "<p><b>Baudrate:</b> " + String(newConfig.crsfBaudrate) + " baud</p>";
            response += "<p><strong>Note: Re-applying configuration requires restarting the device.</strong></p>";
            response += "<a href='/'>&larr; Back to configuration</a>";
            response += "</div></body></html>";

            _server.send(200, "text/html", response);

            // Allow the response to send, then reboot the device to apply changes safely
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
    json += "]";
    json += "}";
    _server.send(200, "application/json", json);
}
