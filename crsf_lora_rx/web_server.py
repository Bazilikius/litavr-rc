import socket
import network
import machine
import json

def url_decode(s):
    res = s.replace("+", " ")
    parts = res.split("%")
    if len(parts) == 1:
        return res
    decoded = parts[0]
    for part in parts[1:]:
        if len(part) >= 2:
            try:
                decoded += chr(int(part[:2], 16)) + part[2:]
            except:
                decoded += "%" + part
        else:
            decoded += "%" + part
    return decoded

class WebServer:
    def __init__(self, config_manager, shared_data):
        self.config_manager = config_manager
        self.shared_data = shared_data
        self.server_socket = None

    def start(self):
        # Configure SoftAP with 25% Tx Power
        ap = network.WLAN(network.AP_IF)
        ap.active(True)
        ap.config(essid="CRSF-Config-RX", password="12345678")

        # Max TX power range is typically 0-78 on ESP32 (approx 19.5dBm max)
        # To limit Tx Power to 25%, we can set it to a lower value in ap.config (e.g. 8)
        try:
            ap.config(txpower=8)
            print("[Web] WiFi softAP 'CRSF-Config-RX' started at 25% TX power.")
        except Exception as e:
            print("[Web] Could not set TX power dynamically:", e)

        print("[Web] softAP IP Address:", ap.ifconfig()[0])

        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind(("", 80))
        self.server_socket.listen(5)
        self.server_socket.setblocking(False)
        print("[Web] Socket server listening on port 80...")

    def handle_client(self):
        if not self.server_socket:
            return
        try:
            client, addr = self.server_socket.accept()
        except OSError:
            # No incoming connection
            return

        try:
            client.settimeout(1.0)
            req = client.recv(1024).decode("utf-8", "ignore")
            if not req:
                client.close()
                return

            lines = req.split("\r\n")
            first_line = lines[0].split()
            if len(first_line) < 2:
                client.close()
                return

            method, path = first_line[0], first_line[1]

            if path == "/status":
                self._send_status(client)
            elif path.startswith("/save"):
                self._send_save(client, req)
            else:
                self._send_root(client)
        except Exception as e:
            print("[Web] Error handling socket request:", e)
        finally:
            try:
                client.close()
            except:
                pass

    def _send_root(self, client):
        config = self.config_manager.config
        html = "<!DOCTYPE html><html><head>"
        html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        html += "<meta charset='UTF-8'>"
        html += "<title>Панель керування CRSF LoRa RX</title>"
        html += "<style>"
        html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; text-align: center; padding: 20px; }"
        html += ".container { background-color: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; max-width: 550px; width: 100%; text-align: left; }"
        html += "h1 { color: #007bff; margin-bottom: 20px; font-size: 24px; text-align: center; }"
        html += "label { display: block; margin: 10px 0 5px; font-weight: bold; }"
        html += "select, input[type='number'] { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 15px; background: #fff; box-sizing: border-box; }"
        html += "input[type='checkbox'] { transform: scale(1.3); margin-right: 10px; vertical-align: middle; }"
        html += ".checkbox-label { display: flex; align-items: center; margin: 15px 0; font-weight: bold; cursor: pointer; }"
        html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; font-weight: bold; margin-top: 15px; }"
        html += "input[type='submit']:hover { background-color: #0056b3; }"
        html += ".panel { background: #eef2f7; border: 1px solid #ddd; padding: 15px; border-radius: 6px; margin-bottom: 20px; }"
        html += ".panel h3 { margin-top: 0; color: #333; }"
        html += ".panel p { margin: 5px 0; font-family: monospace; font-size: 14px; }"
        html += ".status-indicator { font-weight: bold; color: red; }"
        html += ".status-ok { color: green; }"
        html += ".grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 5px; font-family: monospace; font-size: 12px; margin-top: 10px; }"
        html += ".grid-item { background: #ddd; padding: 5px; text-align: center; border-radius: 3px; }"
        html += ".row { display: flex; gap: 10px; }"
        html += ".row div { flex: 1; }"
        html += "hr { border: 0; border-top: 1px solid #ddd; margin: 20px 0; }"
        html += "</style>"
        html += "<script>"
        html += "function syncChannels() {"
        html += "  const allOne = document.getElementById('all_one').checked;"
        html += "  const srvChan = document.getElementById('srv_chan').value;"
        html += "  const boxSelCh = document.getElementById('box_sel_ch');"
        html += "  if (allOne) {"
        html += "    boxSelCh.value = srvChan; boxSelCh.disabled = true;"
        html += "  } else {"
        html += "    boxSelCh.disabled = false;"
        html += "  }"
        html += "}"
        html += ""
        html += "function getSelectedBox(pw) {"
        html += "  if(pw < 1000) pw = 1000;"
        html += "  if(pw > 2000) pw = 2000;"
        html += "  let box = Math.floor((pw - 1000) / 125) + 1;"
        html += "  if(box < 1) box = 1;"
        html += "  if(box > 8) box = 8;"
        html += "  return box;"
        html += "}"
        html += ""
        html += "function updateStatus() {"
        html += "  fetch('/status').then(r => r.json()).then(data => {"
        html += "    document.getElementById('packetCount').innerText = data.packets;"
        html += "    const link = document.getElementById('linkStatus');"
        html += "    if(data.packets > 0) {"
        html += "      link.innerText = 'ONLINE'; link.className = 'status-indicator status-ok';"
        html += "    } else {"
        html += "      link.innerText = 'NO DATA'; link.className = 'status-indicator';"
        html += "    }"
        html += "    document.getElementById('upperSw').innerText = data.upperSw ? 'ВІДКРИТИЙ / Triggered' : 'CLOSED / OK';"
        html += "    document.getElementById('upperSw').className = data.upperSw ? 'status-indicator' : 'status-indicator status-ok';"
        html += "    document.getElementById('lowerSw').innerText = data.lowerSw ? 'ВІДКРИТИЙ / Triggered' : 'CLOSED / OK';"
        html += "    document.getElementById('lowerSw').className = data.lowerSw ? 'status-indicator' : 'status-indicator status-ok';"
        html += "    document.getElementById('servoUpSw').innerText = data.servoUpSw ? 'ВІДКРИТИЙ / Triggered (OVERRIDE UP)' : 'CLOSED / OK';"
        html += "    document.getElementById('servoUpSw').className = data.servoUpSw ? 'status-indicator' : 'status-indicator status-ok';"
        html += "    "
        html += "    const selChIdx = parseInt(document.getElementById('box_sel_ch').value) - 1;"
        html += "    const selChVal = data.channels[selChIdx];"
        html += "    const selBox = getSelectedBox(selChVal);"
        html += "    const myBoxId = " + str(config["boxId"]) + ";"
        html += "    "
        html += "    document.getElementById('activeBox').innerText = selBox + ' (CH' + (selChIdx + 1) + ': ' + selChVal + 'us)';"
        html += "    "
        html += "    const myBoxStatus = document.getElementById('myBoxStatus');"
        html += "    if (selBox === myBoxId) {"
        html += "      myBoxStatus.innerText = 'ВИБРАНО ЦЕЙ МОДУЛЬ (АКТИВНИЙ)'; myBoxStatus.className = 'status-indicator status-ok';"
        html += "    } else {"
        html += "      myBoxStatus.innerText = 'НЕ ВИБРАНО'; myBoxStatus.className = 'status-indicator';"
        html += "    }"
        html += "    "
        html += "    document.getElementById('countdown').innerText = data.countdown;"
        html += "    "
        html += "    const ovr = document.getElementById('overrideAlert');"
        html += "    if (data.override) {"
        html += "      ovr.innerText = 'УВАГА: Кінцевик відкритий! MOSFET та Extra Pin вимкнено!'; ovr.style.color = 'red';"
        html += "    } else {"
        html += "      ovr.innerText = 'Усі кінцевики в нормі (Безпечно)'; ovr.style.color = 'green';"
        html += "    }"
        html += "    for(let i=0; i<16; i++) {"
        html += "      const cell = document.getElementById('ch' + i);"
        html += "      if(cell) cell.innerText = 'CH' + (i+1) + ': ' + data.channels[i];"
        html += "    }"
        html += "  });"
        html += "}"
        html += "window.onload = function() {"
        html += "  syncChannels();"
        html += "  setInterval(updateStatus, 1000);"
        html += "};"
        html += "</script>"
        html += "</head><body>"

        html += "<div class='container'>"
        html += "<h1>Конфігурація CRSF LoRa RX (MicroPython)</h1>"

        # Live status panel
        html += "<div class='panel'>"
        html += "<h3>Монітор стану</h3>"
        html += "<p>Стан зв'язку LoRa: <span id='linkStatus' class='status-indicator'>ОЧІКУВАННЯ...</span></p>"
        html += "<p>Отримано пакетів: <span id='packetCount'>0</span></p>"
        html += "<p>Номер нашого модуля (Box ID): <strong>" + str(config["boxId"]) + "</strong></p>"
        html += "<p>Зараз вибрано модуль: <span id='activeBox' style='font-weight: bold;'>-</span></p>"
        html += "<p>Статус нашого модуля: <span id='myBoxStatus' class='status-indicator'>-</span></p>"
        html += "<p>Верхній кінцевик (GPIO 32): <span id='upperSw' class='status-indicator'>-</span></p>"
        html += "<p>Нижній кінцевик (GPIO 33): <span id='lowerSw' class='status-indicator'>-</span></p>"
        html += "<p>Кінцевик Servo-UP (GPIO 25): <span id='servoUpSw' class='status-indicator'>-</span></p>"
        html += "<p>Час до автоматичної активації Power Key (GPIO 15): <span id='countdown' style='font-weight: bold;'>-</span> сек</p>"
        html += "<p id='overrideAlert' style='font-weight: bold; margin-top: 10px;'>Очікування даних...</p>"
        html += "<strong>Значення каналів:</strong>"
        html += "<div class='grid'>"
        for i in range(16):
            html += "<div class='grid-item' id='ch" + str(i) + "'>CH" + str(i+1) + ": -</div>"
        html += "</div>"
        html += "</div>"

        # Form
        html += "<form action='/save' method='GET' onsubmit='document.getElementById(\"box_sel_ch\").disabled=false;'>"

        # All One Channel Checkbox
        checked_all_one = "checked" if config.get("allOneChannel", 0) else ""
        html += "<label class='checkbox-label'>"
        html += "<input type='checkbox' name='all_one' value='1' id='all_one' onchange='syncChannels()' " + checked_all_one + "> Все одним каналом"
        html += "</label>"

        html += "<hr>"

        # Box config
        html += "<h3>1. Налаштування Модуля (Box ID)</h3>"
        html += "<label for='box_id'>Номер цього ящика (1-8):</label>"
        html += "<select name='box_id' id='box_id'>"
        for i in range(1, 9):
            selected = "selected" if config["boxId"] == i else ""
            html += "<option value='" + str(i) + "' " + selected + ">Ящик №" + str(i) + "</option>"
        html += "</select>"

        html += "<label for='box_sel_ch'>Канал вибору ящиків (1-16):</label>"
        html += "<select name='box_sel_ch' id='box_sel_ch'>"
        for i in range(1, 17):
            selected = "selected" if config["boxSelectChannel"] == i else ""
            html += "<option value='" + str(i) + "' " + selected + ">CH " + str(i) + "</option>"
        html += "</select>"

        html += "<hr>"

        # Servos config
        html += "<h3>2. Налаштування Сервоприводів (GPIO 4, GPIO 13)</h3>"
        html += "<label for='srv_chan'>Канал CRSF:</label>"
        html += "<select name='srv_chan' id='srv_chan' onchange='syncChannels()'>"
        for i in range(1, 17):
            selected = "selected" if config["servoChannel"] == i else ""
            html += "<option value='" + str(i) + "' " + selected + ">CH " + str(i) + "</option>"
        html += "</select>"

        html += "<label for='srv_trig'>Положення тумблера для активації:</label>"
        html += "<select name='srv_trig' id='srv_trig'>"
        for pos in [1000, 1500, 2000]:
            selected = "selected" if config["servoTrigger"] == pos else ""
            html += "<option value='" + str(pos) + "' " + selected + ">" + str(pos) + "</option>"
        html += "</select>"

        checked_inv_l = "checked" if config.get("servoInvertLeft", 0) else ""
        html += "<label class='checkbox-label'>"
        html += "<input type='checkbox' name='srv_inv_l' value='1' id='srv_inv_l' " + checked_inv_l + "> Інвертувати лівий сервопривід (GPIO 4)"
        html += "</label>"

        checked_inv_r = "checked" if config.get("servoInvertRight", 0) else ""
        html += "<label class='checkbox-label'>"
        html += "<input type='checkbox' name='srv_inv' value='1' id='srv_inv' " + checked_inv_r + "> Інвертувати правий сервопривід (GPIO 13)"
        html += "</label>"

        html += "<div class='row'>"
        html += "  <div><label>Min PWM (us):</label><input type='number' name='srv_min' value='" + str(config["servoMin"]) + "' min='500' max='2500'></div>"
        html += "  <div><label>Max PWM (us):</label><input type='number' name='srv_max' value='" + str(config["servoMax"]) + "' min='500' max='2500'></div>"
        html += "</div>"

        html += "<label for='srv_spd'>Швидкість руху сервоприводів (us/сек):</label>"
        html += "<input type='number' name='srv_spd' id='srv_spd' value='" + str(config["servoSpeed"]) + "' min='5' max='5000'>"

        html += "<hr>"

        # Off polarity config
        html += "<h3>3. Сигнал вимкнення виходу (Полярність)</h3>"
        html += "<label for='mos_off'>Який сигнал відправляти на вихід при вимкненні (MOSFET/GPIO 26 та Extra/GPIO 15):</label>"
        html += "<select name='mos_off' id='mos_off'>"
        mos_off_sel_0 = "selected" if config["mosfetOffLevel"] == 0 else ""
        mos_off_sel_1 = "selected" if config["mosfetOffLevel"] == 1 else ""
        html += "<option value='0' " + mos_off_sel_0 + ">LOW / 1000us PWM (Вимкнено = 1000us, Активовано = 1500us/2000us)</option>"
        html += "<option value='1' " + mos_off_sel_1 + ">HIGH / 2000us PWM (Вимкнено = 2000us, Активовано = 1500us/2000us)</option>"
        html += "</select>"

        html += "<hr>"

        # LoRa frequency config
        html += "<h3>4. Системні налаштування</h3>"
        html += "<label for='lora_freq'>Частота LoRa (Hz):</label>"
        html += "<input type='number' name='lora_freq' id='lora_freq' value='" + str(config["loraFreq"]) + "' min='100000000' max='1000000000'>"

        html += "<input type='submit' value='Зберегти та перезавантажити'>"
        html += "</form>"
        html += "</div></body></html>"

        response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" + html
        client.send(response.encode("utf-8"))

    def _send_save(self, client, req):
        # Extract query parameters from request path
        # Format: GET /save?box_id=1&box_sel_ch=15... HTTP/1.1
        try:
            path_line = req.split("\r\n")[0]
            path = path_line.split()[1]
            query_str = path.split("?")[1]
            params = {}
            for item in query_str.split("&"):
                k, v = item.split("=")
                params[k] = url_decode(v)

            # Update configuration
            new_cfg = {
                "boxId": int(params.get("box_id", 1)),
                "servoChannel": int(params.get("srv_chan", 16)),
                "servoTrigger": int(params.get("srv_trig", 1500)),
                "servoInvertLeft": 1 if "srv_inv_l" in params else 0,
                "servoInvertRight": 1 if "srv_inv" in params else 0,
                "servoMin": int(params.get("srv_min", 1500)),
                "servoMax": int(params.get("srv_max", 2000)),
                "servoSpeed": int(params.get("srv_spd", 100)),
                "loraFreq": int(params.get("lora_freq", 433000000)),
                "allOneChannel": 1 if "all_one" in params else 0,
                "mosfetOffLevel": int(params.get("mos_off", 0))
            }
            if not new_cfg["allOneChannel"]:
                new_cfg["boxSelectChannel"] = int(params.get("box_sel_ch", 15))
            else:
                new_cfg["boxSelectChannel"] = new_cfg["servoChannel"]

            self.config_manager.save(new_cfg)

            html = "<!DOCTYPE html><html><head>"
            html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
            html += "<meta charset='UTF-8'><title>Налаштування збережено</title>"
            html += "<style>body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; padding: 50px; }"
            html += ".message { background: #d4edda; color: #155724; padding: 20px; border-radius: 8px; border: 1px solid #c3e6cb; display: inline-block; max-width: 400px; text-align: left; }"
            html += "a { display: inline-block; margin-top: 15px; color: #007bff; text-decoration: none; font-weight: bold; }</style></head><body>"
            html += "<div class='message'><h2>Налаштування успішно збережено!</h2>"
            html += "<p>Пристрій перезавантажується для застосування нових параметрів.</p>"
            html += "<a href='/'>&larr; Назад до панелі керування</a></div></body></html>"

            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" + html
            client.send(response.encode("utf-8"))

            # Reboot ESP32 dynamically
            time.sleep_ms(1000)
            machine.reset()
        except Exception as e:
            print("[Web] Save error:", e)
            client.send("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\nInvalid parameters".encode())

    def _send_status(self, client):
        countdown = 60
        lowered_ts = self.shared_data.get("loweredTimestamp", 0)
        if lowered_ts != 0:
            elapsed = time.ticks_diff(time.ticks_ms(), lowered_ts) // 1000
            countdown = max(0, 60 - elapsed)

        status = {
            "packets": self.shared_data.get("packetCount", 0),
            "upperSw": 1 if self.shared_data.get("upperSw", False) else 0,
            "lowerSw": 1 if self.shared_data.get("lowerSw", False) else 0,
            "servoUpSw": 1 if self.shared_data.get("servoUpSw", False) else 0,
            "override": 1 if self.shared_data.get("override", False) else 0,
            "countdown": countdown,
            "channels": self.shared_data.get("channels", [1500]*16)
        }

        body = json.dumps(status)
        response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " + str(len(body)) + "\r\n\r\n" + body
        client.send(response.encode("utf-8"))
