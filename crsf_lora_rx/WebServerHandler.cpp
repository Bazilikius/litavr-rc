#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(ConfigManager &configManager, uint32_t &packetCount, uint16_t *channels, bool &upperSw, bool &lowerSw, bool &servoUpSw, bool &overrideActive, uint32_t &loweredTimestamp)
    : _configManager(configManager), _packetCount(packetCount), _channels(channels), _upperSw(upperSw), _lowerSw(lowerSw), _servoUpSw(servoUpSw), _overrideActive(overrideActive), _loweredTimestamp(loweredTimestamp), _server(80) {}

void WebServerHandler::begin() {
    // Start AP Mode
    WiFi.softAP("CRSF-Config-RX", "12345678");

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
    html += "function syncChannels() {";
    html += "  const allOne = document.getElementById('all_one').checked;";
    html += "  const srvChan = document.getElementById('srv_chan').value;";
    html += "  ";
    html += "  const boxSelCh = document.getElementById('box_sel_ch');";
    html += "  ";
    html += "  if (allOne) {";
    html += "    boxSelCh.value = srvChan; boxSelCh.disabled = true;";
    html += "  } else {";
    html += "    boxSelCh.disabled = false;";
    html += "  }";
    html += "}";
    html += "";
    html += "function getSelectedBox(pw) {";
    html += "  if(pw < 1000) pw = 1000;";
    html += "  if(pw > 2000) pw = 2000;";
    html += "  let box = Math.floor((pw - 1000) / 125) + 1;";
    html += "  if(box < 1) box = 1;";
    html += "  if(box > 8) box = 8;";
    html += "  return box;";
    html += "}";
    html += "";
    html += "function updateStatus() {";
    html += "  fetch('/status').then(r => r.json()).then(data => {";
    html += "    document.getElementById('packetCount').innerText = data.packets;";
    html += "    const link = document.getElementById('linkStatus');";
    html += "    if(data.packets > 0) {";
    html += "      link.innerText = 'ONLINE'; link.className = 'status-indicator status-ok';";
    html += "    } else {";
    html += "      link.innerText = 'NO DATA'; link.className = 'status-indicator';";
    html += "    }";
    html += "    document.getElementById('upperSw').innerText = data.upperSw ? 'ВІДКРИТИЙ / Triggered' : 'CLOSED / OK';";
    html += "    document.getElementById('upperSw').className = data.upperSw ? 'status-indicator' : 'status-indicator status-ok';";
    html += "    document.getElementById('lowerSw').innerText = data.lowerSw ? 'ВІДКРИТИЙ / Triggered' : 'CLOSED / OK';";
    html += "    document.getElementById('lowerSw').className = data.lowerSw ? 'status-indicator' : 'status-indicator status-ok';";
    html += "    document.getElementById('servoUpSw').innerText = data.servoUpSw ? 'ВІДКРИТИЙ / Triggered (OVERRIDE UP)' : 'CLOSED / OK';";
    html += "    document.getElementById('servoUpSw').className = data.servoUpSw ? 'status-indicator' : 'status-indicator status-ok';";
    html += "    ";
    html += "    const selChIdx = parseInt(document.getElementById('box_sel_ch').value) - 1;";
    html += "    const selChVal = data.channels[selChIdx];";
    html += "    const selBox = getSelectedBox(selChVal);";
    html += "    const myBoxId = " + String(config.boxId) + ";";
    html += "    ";
    html += "    document.getElementById('activeBox').innerText = selBox + ' (CH' + (selChIdx + 1) + ': ' + selChVal + 'us)';";
    html += "    ";
    html += "    const myBoxStatus = document.getElementById('myBoxStatus');";
    html += "    if (selBox === myBoxId) {";
    html += "      myBoxStatus.innerText = 'ВИБРАНО ЦЕЙ МОДУЛЬ (АКТИВНИЙ)'; myBoxStatus.className = 'status-indicator status-ok';";
    html += "    } else {";
    html += "      myBoxStatus.innerText = 'НЕ ВИБРАНО'; myBoxStatus.className = 'status-indicator';";
    html += "    }";
    html += "    ";
    html += "    document.getElementById('countdown').innerText = data.countdown;";
    html += "    ";
    html += "    const ovr = document.getElementById('overrideAlert');";
    html += "    if (data.override) {";
    html += "      ovr.innerText = 'УВАГА: Кінцевик відкритий! MOSFET та Extra Pin вимкнено!'; ovr.style.color = 'red';";
    html += "    } else {";
    html += "      ovr.innerText = 'Усі кінцевики в нормі (Безпечно)'; ovr.style.color = 'green';";
    html += "    }";
    html += "    for(let i=0; i<16; i++) {";
    html += "      const cell = document.getElementById('ch' + i);";
    html += "      if(cell) cell.innerText = 'CH' + (i+1) + ': ' + data.channels[i];";
    html += "    }";
    html += "  });";
    html += "}";
    html += "window.onload = function() {";
    html += "  syncChannels();";
    html += "  setInterval(updateStatus, 1000);";
    html += "};";
    html += "</script>";
    html += "</head><body>";

    html += "<div class='container'>";
    html += "<h1>Конфігурація CRSF LoRa RX (Mesh 4)</h1>";

    // Live status panel
    html += "<div class='panel'>";
    html += "<h3>Монітор стану</h3>";
    html += "<p>Стан зв'язку LoRa: <span id='linkStatus' class='status-indicator'>ОЧІКУВАННЯ...</span></p>";
    html += "<p>Отримано пакетів: <span id='packetCount'>0</span></p>";
    html += "<p>Номер нашого модуля (Box ID): <strong>" + String(config.boxId) + "</strong></p>";
    html += "<p>Зараз вибрано модуль: <span id='activeBox' style='font-weight: bold;'>-</span></p>";
    html += "<p>Статус нашого модуля: <span id='myBoxStatus' class='status-indicator'>-</span></p>";
    html += "<p>Верхній кінцевик (GPIO 32): <span id='upperSw' class='status-indicator'>-</span></p>";
    html += "<p>Нижній кінцевик (GPIO 33): <span id='lowerSw' class='status-indicator'>-</span></p>";
    html += "<p>Кінцевик Servo-UP (GPIO 25): <span id='servoUpSw' class='status-indicator'>-</span></p>";
    html += "<p>Час до автоматичної активації Power Key (GPIO 15): <span id='countdown' style='font-weight: bold;'>-</span> сек</p>";
    html += "<p id='overrideAlert' style='font-weight: bold; margin-top: 10px;'>Очікування даних...</p>";
    html += "<strong>Значення каналів:</strong>";
    html += "<div class='grid'>";
    for (int i = 0; i < 16; i++) {
        html += "<div class='grid-item' id='ch" + String(i) + "'>CH" + String(i+1) + ": -</div>";
    }
    html += "</div>";
    html += "</div>";

    // Form
    html += "<form action='/save' method='POST' onsubmit='document.getElementById(\"box_sel_ch\").disabled=false;'>";

    // Checkbox: "Все одним каналом"
    html += "<label class='checkbox-label'>";
    String checkedAllOne = config.allOneChannel ? "checked" : "";
    html += "<input type='checkbox' name='all_one' value='1' id='all_one' onchange='syncChannels()' " + checkedAllOne + "> Все одним каналом (копіює канал сервоприводу на вибір ящика)";
    html += "</label>";

    html += "<hr>";

    // 1. Box Selector ID (1-8)
    html += "<h3>1. Налаштування Модуля (Box ID)</h3>";
    html += "<label for='box_id'>Номер цього ящика (1-8):</label>";
    html += "<select name='box_id' id='box_id'>";
    for (int i = 1; i <= 8; i++) {
        String selected = (config.boxId == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">Ящик №" + String(i) + "</option>";
    }
    html += "</select>";

    html += "<label for='box_sel_ch'>Канал вибору ящиків (1-16):</label>";
    html += "<select name='box_sel_ch' id='box_sel_ch'>";
    for (int i = 1; i <= 16; i++) {
        String selected = (config.boxSelectChannel == i) ? "selected" : "";
        html += "<option value='" + String(i) + "' " + selected + ">CH " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<hr>";

    // 2. Servos Config
    html += "<h3>2. Налаштування Сервоприводів (GPIO 4, GPIO 13)</h3>";
    html += "<label for='srv_chan'>Канал CRSF (Керування):</label>";
    html += "<select name='srv_chan' id='srv_chan' onchange='syncChannels()'>";
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
    String checkedInvR = config.servoInvertRight ? "checked" : "";
    html += "<input type='checkbox' name='srv_inv' value='1' id='srv_inv' " + checkedInvR + "> Інвертувати правий сервопривід (GPIO 13)";
    html += "</label>";

    html += "<div class='row'>";
    html += "  <div><label>Min PWM (us):</label><input type='number' name='srv_min' value='" + String(config.servoMin) + "' min='500' max='2500'></div>";
    html += "  <div><label>Max PWM (us):</label><input type='number' name='srv_max' value='" + String(config.servoMax) + "' min='500' max='2500'></div>";
    html += "</div>";

    html += "<label for='srv_spd'>Швидкість руху сервоприводів (us/сек):</label>";
    html += "<input type='number' name='srv_spd' id='srv_spd' value='" + String(config.servoSpeed) + "' min='5' max='5000'>";

    html += "<hr>";

    // 3. Limit Switches Count Configuration
    html += "<h3>3. Кількість кінцевиків та безпека</h3>";
    html += "<label class='checkbox-label'>";
    String checkedUse3Sw = config.useThreeSwitches ? "checked" : "";
    html += "<input type='checkbox' name='use_3_sw' value='1' id='use_3_sw' " + checkedUse3Sw + "> Використовувати 3 кінцевики (включаючи GPIO 25)";
    html += "</label>";
    html += "<p style='font-size: 13px; color: #666; margin-top: -10px; margin-bottom: 15px;'>* Якщо вимкнено (рекомендовано для 2 кінцевиків), система повністю ігнорує 3-й кінцевик на GPIO 25, що дозволяє таймеру затримки GPIO 15 успішно запускатися без підключеного оверрайду!</p>";

    html += "<hr>";

    // 4. Signal Level Settings
    html += "<h3>4. Сигнали вимкнення виходів (Полярність)</h3>";

    html += "<label for='mos_off'>Сигнал вимкнення MOSFET (GPIO 26):</label>";
    html += "<select name='mos_off' id='mos_off'>";
    String mosOffSel0 = (config.mosfetOffLevel == 0) ? "selected" : "";
    String mosOffSel1 = (config.mosfetOffLevel == 1) ? "selected" : "";
    html += "<option value='0' " + mosOffSel0 + ">LOW / 1000us PWM (Вимкнено = 1000us, Активовано = " + String(config.mosfetActiveWidth) + "us)</option>";
    html += "<option value='1' " + mosOffSel1 + ">HIGH / 2000us PWM (Вимкнено = 2000us, Активовано = " + String(config.mosfetActiveWidth) + "us)</option>";
    html += "</select>";

    html += "<label for='mos_act'>Сигнал активації MOSFET (GPIO 26):</label>";
    html += "<select name='mos_act' id='mos_act'>";
    String mosActSel2000 = (config.mosfetActiveWidth == 2000) ? "selected" : "";
    String mosActSel2500 = (config.mosfetActiveWidth == 2500) ? "selected" : "";
    html += "<option value='2000' " + mosActSel2000 + ">2000us PWM</option>";
    html += "<option value='2500' " + mosActSel2500 + ">2500us PWM</option>";
    html += "</select>";

    html += "<label for='p_key_off'>Сигнал вимкнення Power Key (GPIO 15):</label>";
    html += "<select name='p_key_off' id='p_key_off'>";
    String pKeyOffSel0 = (config.powerKeyOffLevel == 0) ? "selected" : "";
    String pKeyOffSel1 = (config.powerKeyOffLevel == 1) ? "selected" : "";
    html += "<option value='0' " + pKeyOffSel0 + ">LOW / 1000us PWM (Вимкнено = 1000us, Активовано = 2000us)</option>";
    html += "<option value='1' " + pKeyOffSel1 + ">HIGH / 2000us PWM (Вимкнено = 2000us, Активовано = 2000us)</option>";
    html += "</select>";

    html += "<hr>";

    // 5. System Config
    html += "<h3>5. Системні налаштування</h3>";
    html += "<label for='lora_freq'>Частота LoRa (Hz):</label>";
    html += "<input type='number' name='lora_freq' id='lora_freq' value='" + String(config.loraFreq) + "' min='100000000' max='1000000000'>";

    html += "<input type='submit' value='Зберегти та перезавантажити'>";
    html += "</form>";
    html += "</div>";

    html += "</body></html>";

    _server.send(200, "text/html", html);
}

void WebServerHandler::_handleSave() {
    if (_server.hasArg("box_id") && _server.hasArg("box_sel_ch") &&
        _server.hasArg("srv_chan") && _server.hasArg("srv_trig") &&
        _server.hasArg("srv_min") && _server.hasArg("srv_max") &&
        _server.hasArg("srv_spd") && _server.hasArg("mos_off") &&
        _server.hasArg("mos_act") &&
        _server.hasArg("p_key_off") && _server.hasArg("lora_freq")) {

        RxConfig newConfig;
        newConfig.boxId = _server.arg("box_id").toInt();
        newConfig.servoChannel = _server.arg("srv_chan").toInt();
        newConfig.servoTrigger = _server.arg("srv_trig").toInt();
        newConfig.servoInvertLeft = _server.hasArg("srv_inv_l") ? 1 : 0;
        newConfig.servoInvertRight = _server.hasArg("srv_inv") ? 1 : 0;
        newConfig.servoMin = _server.arg("srv_min").toInt();
        newConfig.servoMax = _server.arg("srv_max").toInt();
        newConfig.servoSpeed = _server.arg("srv_spd").toInt();
        newConfig.loraFreq = _server.arg("lora_freq").toInt();

        newConfig.allOneChannel = _server.hasArg("all_one") ? 1 : 0;

        if (!newConfig.allOneChannel) {
            newConfig.boxSelectChannel = _server.arg("box_sel_ch").toInt();
        } else {
            newConfig.boxSelectChannel = newConfig.servoChannel;
        }

        newConfig.mosfetOffLevel = _server.arg("mos_off").toInt();
        newConfig.mosfetActiveWidth = _server.arg("mos_act").toInt();
        newConfig.powerKeyOffLevel = _server.arg("p_key_off").toInt();
        newConfig.useThreeSwitches = _server.hasArg("use_3_sw") ? 1 : 0;

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
    uint32_t remaining = 60;
    if (_loweredTimestamp != 0) {
        uint32_t elapsed = (millis() - _loweredTimestamp) / 1000;
        remaining = (elapsed < 60) ? (60 - elapsed) : 0;
    }

    String json = "{";
    json += "\"packets\":" + String(_packetCount) + ",";
    json += "\"upperSw\":" + String(_upperSw ? 1 : 0) + ",";
    json += "\"lowerSw\":" + String(_lowerSw ? 1 : 0) + ",";
    json += "\"servoUpSw\":" + String(_servoUpSw ? 1 : 0) + ",";
    json += "\"override\":" + String(_overrideActive ? 1 : 0) + ",";
    json += "\"countdown\":" + String(remaining) + ",";
    json += "\"channels\":[";
    for (int i = 0; i < 16; i++) {
        json += String(_channels[i]);
        if (i < 15) json += ",";
    }
    json += "]";
    json += "}";
    _server.send(200, "application/json", json);
}
