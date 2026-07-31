#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(ConfigManager &configManager, uint32_t &packetCount, uint16_t *channels, bool &upperSw, bool &lowerSw, bool &servoUpSw, bool &overrideActive)
    : _configManager(configManager), _packetCount(packetCount), _channels(channels), _upperSw(upperSw), _lowerSw(lowerSw), _servoUpSw(servoUpSw), _overrideActive(overrideActive), _server(80) {}

void WebServerHandler::begin() {
    // Start AP Mode
    WiFi.softAP("CRSF-Config-RX", "12345678");

    // Reduce WiFi Tx Power to 25% (WIFI_POWER_5dBm)
    WiFi.setTxPower(WIFI_POWER_5dBm);

    Serial.println("[Web] WiFi Access Point Started: SSID='CRSF-Config-RX', Password='12345678'");
    Serial.printf("[Web] WiFi Transmit Power limited to 5dBm (25%% power).\n");
    Serial.print("[Web] IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Setup routes
    _server.on("/", std::bind(&WebServerHandler::_handleRoot, this));
    _server.on("/save", std::bind(&WebServerHandler::_handleSave, this));
    _server.on("/status", std::bind(&WebServerHandler::_handleStatus, this));

    _server.begin();
    Serial.println("[Web] HTTP Web Server Started.");
}

void WebServerHandler::handleClient() {
    _server.handleClient();
}

void WebServerHandler::_handleRoot() {
    RxConfig config = _configManager.getConfig();

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<meta charset='UTF-8'>";
    html += "<title>Панель керування CRSF LoRa RX</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; text-align: center; padding: 20px; }";
    html += ".container { background-color: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; max-width: 550px; width: 100%; text-align: left; }";
    html += "h1 { color: #007bff; margin-bottom: 20px; font-size: 24px; text-align: center; }";
    html += "label { display: block; margin: 10px 0 5px; font-weight: bold; }";
    html += "select, input[type='number'] { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 15px; background: #fff; box-sizing: border-box; }";
    html += "input[type='checkbox'] { transform: scale(1.3); margin-right: 10px; vertical-align: middle; }";
    html += ".checkbox-label { display: flex; align-items: center; margin: 15px 0; font-weight: bold; cursor: pointer; }";
    html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; font-weight: bold; margin-top: 15px; }";
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
    html += "    document.getElementById('packetCount').innerText = data.packets;";
    html += "    const link = document.getElementById('linkStatus');";
    html += "    if(data.packets > 0) {";
    html += "      link.innerText = 'ONLINE'; link.className = 'status-indicator status-ok';";
    html += "    } else {";
    html += "      link.innerText = 'NO DATA'; link.className = 'status-indicator';";
    html += "    }";
    html += "    document.getElementById('upperSw').innerText = data.upperSw ? 'АКТИВНИЙ / triggered' : 'CLOSED / OK';";
    html += "    document.getElementById('upperSw').className = data.upperSw ? 'status-indicator' : 'status-indicator status-ok';";
    html += "    document.getElementById('lowerSw').innerText = data.lowerSw ? 'АКТИВНИЙ / triggered' : 'CLOSED / OK';";
    html += "    document.getElementById('lowerSw').className = data.lowerSw ? 'status-indicator' : 'status-indicator status-ok';";
    html += "    document.getElementById('servoUpSw').innerText = data.servoUpSw ? 'АКТИВНИЙ / triggered (Servos UP)' : 'CLOSED / OK';";
    html += "    document.getElementById('servoUpSw').className = data.servoUpSw ? 'status-indicator' : 'status-indicator status-ok';";
    html += "    for(let i=0; i<16; i++) {";
    html += "      const cell = document.getElementById('ch' + i);";
    html += "      if(cell) cell.innerText = 'CH' + (i+1) + ': ' + data.channels[i];";
    html += "    }";
    html += "  });";
    html += "}";
    html += "window.onload = function() {";
    html += "  setInterval(updateStatus, 1000);";
    html += "};";
    html += "</script>";
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<h1>Конфігурація CRSF LoRa RX</h1>";

    // Live monitor
    html += "<div class='panel'>";
    html += "<h3>Монітор стану</h3>";
    html += "<p>Стан зв'язку LoRa: <span id='linkStatus' class='status-indicator'>ОЧІКУВАННЯ...</span></p>";
    html += "<p>Отримано пакетів: <span id='packetCount'>0</span></p>";
    html += "<p>Верхній кінцевик (GPIO 32): <span id='upperSw' class='status-indicator'>-</span></p>";
    html += "<p>Нижній кінцевик (GPIO 33): <span id='lowerSw' class='status-indicator'>-</span></p>";
    html += "<p>Кінцевик Servo UP (GPIO 25): <span id='servoUpSw' class='status-indicator'>-</span></p>";
    html += "<strong>Значення каналів:</strong>";
    html += "<div class='grid'>";
    for (int i = 0; i < 16; i++) {
        html += "<div class='grid-item' id='ch" + String(i) + "'>CH" + String(i+1) + ": -</div>";
    }
    html += "</div>";
    html += "</div>";

    // Form
    html += "<form action='/save' method='POST'>";

    // 1. Servos Config
    html += "<h3>1. Налаштування Сервоприводів (GPIO 4, GPIO 13)</h3>";
    html += "<label for='srv_chan'>Канал CRSF:</label>";
    html += "<select name='srv_chan' id='srv_chan'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.servoChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<label for='srv_trig'>Положення тумблера для активації:</label>";
    html += "<select name='srv_trig' id='srv_trig'>";
    uint16_t positions[] = {1000, 1500, 2000};
    for (int i = 0; i < 3; i++) {
        String selected = (config.servoTrigger == positions[i]) ? "selected" : "";
        html += "<option value='" + String(positions[i]) + "' " + selected + ">" + String(positions[i]) + "</option>";
    }
    html += "</select>";

    html += "<label class='checkbox-label'>";
    String checkedInvL = config.servoInvertLeft ? "checked" : "";
    html += "<input type='checkbox' name='srv_inv_l' value='1' id='srv_inv_l' " + checkedInvL + "> Інвертувати лівий сервопривід (GPIO 4)";
    html += "</label>";

    html += "<label class='checkbox-label'>";
    String checkedInv = config.servoInvertRight ? "checked" : "";
    html += "<input type='checkbox' name='srv_inv' value='1' id='srv_inv' " + checkedInv + "> Інвертувати правий сервопривід (GPIO 13)";
    html += "</label>";

    html += "<div class='row'>";
    html += "  <div><label>Min PWM (us):</label><input type='number' name='srv_min' value='" + String(config.servoMin) + "' min='500' max='2500'></div>";
    html += "  <div><label>Max PWM (us):</label><input type='number' name='srv_max' value='" + String(config.servoMax) + "' min='500' max='2500'></div>";
    html += "</div>";

    html += "<hr>";

    // 2. Limit Switch Polarity Settings
    html += "<h3>2. Налаштування Кінцевих Вимикачів</h3>";
    html += "<p style='font-size: 14px; color: #666; margin-bottom: 15px;'>Тут ви можете налаштувати активний рівень (відкритий чи закритий контакт при спрацюванні) для кожного з 3 кінцевиків:</p>";

    html += "<label for='up_pol'>Верхній кінцевик (GPIO 32):</label>";
    html += "<select name='up_pol' id='up_pol'>";
    String upSel0 = (config.upperSwPolarity == 0) ? "selected" : "";
    String upSel1 = (config.upperSwPolarity == 1) ? "selected" : "";
    html += "<option value='0' " + upSel0 + ">Нормально закритий (LOW / Triggered when closed to GND)</option>";
    html += "<option value='1' " + upSel1 + ">Нормально відкритий (HIGH / Triggered when open/disconnected)</option>";
    html += "</select>";

    html += "<label for='lo_pol'>Нижній кінцевик (GPIO 33):</label>";
    html += "<select name='lo_pol' id='lo_pol'>";
    String loSel0 = (config.lowerSwPolarity == 0) ? "selected" : "";
    String loSel1 = (config.lowerSwPolarity == 1) ? "selected" : "";
    html += "<option value='0' " + loSel0 + ">Нормально закритий (LOW / Triggered when closed to GND)</option>";
    html += "<option value='1' " + loSel1 + ">Нормально відкритий (HIGH / Triggered when open/disconnected)</option>";
    html += "</select>";

    html += "<label for='sv_pol'>Кінцевик Servo UP (GPIO 25):</label>";
    html += "<select name='sv_pol' id='sv_pol'>";
    String svSel0 = (config.servoUpSwPolarity == 0) ? "selected" : "";
    String svSel1 = (config.servoUpSwPolarity == 1) ? "selected" : "";
    html += "<option value='0' " + svSel0 + ">Нормально закритий (LOW / Triggered when closed to GND)</option>";
    html += "<option value='1' " + svSel1 + ">Нормально відкритий (HIGH / Triggered when open/disconnected)</option>";
    html += "</select>";

    html += "<hr>";

    // 3. Global settings
    html += "<h3>3. Системні налаштування</h3>";
    html += "<p style='font-size: 14px; color: #666; margin-bottom: 15px;'>Для активації вихідного ключа живлення (GPIO 15) після 60 секунд затримки, усі три кінцевики мають перебувати в безпечному (Closed/OK) стані.</p>";

    html += "<label for='lora_freq'>Частота LoRa (Hz):</label>";
    html += "<input type='number' name='lora_freq' id='lora_freq' value='" + String(config.loraFreq) + "' min='100000000' max='1000000000'>";

    html += "<input type='submit' value='Зберегти та перезавантажити'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";

    _server.send(200, "text/html", html);
}

void WebServerHandler::_handleSave() {
    if (_server.hasArg("srv_chan") && _server.hasArg("srv_trig") &&
        _server.hasArg("srv_min") && _server.hasArg("srv_max") &&
        _server.hasArg("up_pol") && _server.hasArg("lo_pol") &&
        _server.hasArg("sv_pol") && _server.hasArg("lora_freq")) {

        RxConfig newConfig;
        newConfig.servoChannel = _server.arg("srv_chan").toInt();
        newConfig.servoTrigger = _server.arg("srv_trig").toInt();
        newConfig.servoInvertLeft = _server.hasArg("srv_inv_l") ? 1 : 0;
        newConfig.servoInvertRight = _server.hasArg("srv_inv") ? 1 : 0;
        newConfig.servoMin = _server.arg("srv_min").toInt();
        newConfig.servoMax = _server.arg("srv_max").toInt();
        newConfig.loraFreq = _server.arg("lora_freq").toInt();

        newConfig.upperSwPolarity = _server.arg("up_pol").toInt();
        newConfig.lowerSwPolarity = _server.arg("lo_pol").toInt();
        newConfig.servoUpSwPolarity = _server.arg("sv_pol").toInt();

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
        response += "<p>Пристрій перезавантажується для застосування нових параметрів.</p>";
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
    json += "\"packets\":" + String(_packetCount) + ",";
    json += "\"upperSw\":" + String(_upperSw ? 1 : 0) + ",";
    json += "\"lowerSw\":" + String(_lowerSw ? 1 : 0) + ",";
    json += "\"servoUpSw\":" + String(_servoUpSw ? 1 : 0) + ",";
    json += "\"override\":" + String(_overrideActive ? 1 : 0) + ",";
    json += "\"channels\":[";
    for (int i = 0; i < 16; i++) {
        json += String(_channels[i]);
        if (i < 15) json += ",";
    }
    json += "]";
    json += "}";
    _server.send(200, "application/json", json);
}
