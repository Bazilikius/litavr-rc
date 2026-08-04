#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(ConfigManager &configManager, uint32_t &byteCount, uint32_t &packetCount, uint32_t &loraSentCount)
    : _configManager(configManager), _byteCount(byteCount), _packetCount(packetCount), _loraSentCount(loraSentCount), _server(80) {}

void WebServerHandler::begin() {
    // Start AP Mode for Transmitter Config
    WiFi.softAP("CRSF-Config-TX", "12345678");

    // Reduce WiFi Tx Power to 25% (WIFI_POWER_5dBm)
    WiFi.setTxPower(WIFI_POWER_5dBm);

    // Setup routes
    _server.on("/", std::bind(&WebServerHandler::_handleRoot, this));
    _server.on("/save", std::bind(&WebServerHandler::_handleSave, this));
    _server.on("/status", std::bind(&WebServerHandler::_handleStatus, this));

    _server.begin();
}

void WebServerHandler::handleClient() {
    _server.handleClient();
}

void WebServerHandler::_handleRoot() {
    TxConfig config = _configManager.getConfig();

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<meta charset='UTF-8'>";
    html += "<title>Панель керування CRSF LoRa TX</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; text-align: center; padding: 20px; }";
    html += ".container { background-color: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; max-width: 500px; width: 100%; text-align: left; }";
    html += "h1 { color: #007bff; margin-bottom: 20px; font-size: 24px; text-align: center; }";
    html += "label { display: block; margin: 10px 0 5px; font-weight: bold; }";
    html += "select, input[type='number'] { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 15px; background: #fff; box-sizing: border-box; }";
    html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; font-weight: bold; margin-top: 15px; }";
    html += "input[type='submit']:hover { background-color: #0056b3; }";
    html += ".panel { background: #eef2f7; border: 1px solid #ddd; padding: 15px; border-radius: 6px; margin-bottom: 20px; }";
    html += ".panel h3 { margin-top: 0; color: #333; }";
    html += ".panel p { margin: 5px 0; font-family: monospace; font-size: 14px; }";
    html += "</style>";
    html += "<script>";
    html += "function updateStatus() {";
    html += "  fetch('/status').then(r => r.json()).then(data => {";
    html += "    document.getElementById('byteCount').innerText = data.bytes;";
    html += "    document.getElementById('packetCount').innerText = data.packets;";
    html += "    document.getElementById('loraSent').innerText = data.loraSent;";
    html += "  });";
    html += "}";
    html += "window.onload = function() {";
    html += "  setInterval(updateStatus, 1000);";
    html += "};";
    html += "</script>";
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<h1>Конфігурація CRSF LoRa TX (Передавач)</h1>";

    // Status Panel
    html += "<div class='panel'>";
    html += "<h3>Монітор стану</h3>";
    html += "<p>Отримано байт від пульта: <span id='byteCount'>" + String(_byteCount) + "</span></p>";
    html += "<p>Оброблено пакетів CRSF: <span id='packetCount'>" + String(_packetCount) + "</span></p>";
    html += "<p>Відправлено пакетів LoRa: <span id='loraSent'>" + String(_loraSentCount) + "</span></p>";
    html += "</div>";

    // Form
    html += "<form action='/save' method='POST'>";

    // CRSF Baudrate
    html += "<label for='crsf_baud'>Швидкість CRSF (Baudrate):</label>";
    html += "<select name='crsf_baud' id='crsf_baud'>";
    uint32_t bauds[] = {115200, 400000, 420000};
    for (int i = 0; i < 3; i++) {
        String selected = (config.crsfBaudrate == bauds[i]) ? "selected" : "";
        html += "<option value='" + String(bauds[i]) + "' " + selected + ">" + String(bauds[i]) + " baud</option>";
    }
    html += "</select>";

    // LoRa Frequency
    html += "<label for='lora_freq'>Частота LoRa (Hz):</label>";
    html += "<input type='number' name='lora_freq' id='lora_freq' value='" + String(config.loraFreq) + "' min='100000000' max='1000000000'>";

    html += "<input type='submit' value='Зберегти та перезавантажити'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";

    _server.send(200, "text/html", html);
}

void WebServerHandler::_handleSave() {
    if (_server.hasArg("crsf_baud") && _server.hasArg("lora_freq")) {
        TxConfig newConfig;
        newConfig.crsfBaudrate = _server.arg("crsf_baud").toInt();
        newConfig.loraFreq = _server.arg("lora_freq").toInt();

        _configManager.saveConfig(newConfig);

        String response = "<!DOCTYPE html><html><head>";
        response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
        response += "<meta charset='UTF-8'>";
        response += "<title>Налаштування збережено</title>";
        response += "<style>";
        response += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; padding: 50px; }";
        response += ".message { background: #d4edda; color: #155724; padding: 20px; border-radius: 8px; border: 1px solid #c3e6cb; display: inline-block; max-width: 400px; text-align: left; }";
        response += "a { display: inline-block; margin-top: 15px; color: #007bff; text-decoration: none; font-weight: bold; }";
        response += "</style></head><body>";
        response += "<div class='message'>";
        response += "<h2>Налаштування успішно збережено!</h2>";
        response += "<p>Передавач перезавантажується для застосування нових параметрів.</p>";
        response += "<a href='/'>&larr; Назад до панелі керування</a>";
        response += "</div></body></html>";

        _server.send(200, "text/html", response);

        delay(1000);
        ESP.restart();
        return;
    }
    _server.send(400, "text/plain", "Invalid Parameters.");
}

void WebServerHandler::_handleStatus() {
    String json = "{";
    json += "\"bytes\":" + String(_byteCount) + ",";
    json += "\"packets\":" + String(_packetCount) + ",";
    json += "\"loraSent\":" + String(_loraSentCount);
    json += "}";
    _server.send(200, "application/json", json);
}
