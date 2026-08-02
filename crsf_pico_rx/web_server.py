import network
import socket
import time
import json

class WebServer:
    def __init__(self, config_manager, parser, stats):
        self.config_manager = config_manager
        self.parser = parser
        self.stats = stats # dictionary with bytes, packets keys
        self.ap_active = False
        self.server_socket = None

    def begin(self):
        try:
            self.ap = network.WLAN(network.AP_IF)
            self.ap.active(True)
            self.ap.config(essid="CRSF-Config", password="12345678")
            self.ap_active = True
            print("WiFi Access Point Started: SSID='CRSF-Config', Password='12345678'")
            print("IP Address:", self.ap.ifconfig()[0])

            # Open socket on port 80
            self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.server_socket.bind(('', 80))
            self.server_socket.listen(5)
            # Set non-blocking socket to not stall the main parse loop
            self.server_socket.setblocking(False)
            print("HTTP Web Server Started on Port 80.")
        except Exception as e:
            print("WebServer: WiFi AP / socket init failed (possibly standard Pico):", e)
            self.ap_active = False

    def handle_client(self):
        if not self.ap_active or not self.server_socket:
            return

        try:
            conn, addr = self.server_socket.accept()
        except OSError:
            # No client connected
            return

        try:
            conn.settimeout(0.5)
            request_bytes = b""
            while True:
                try:
                    chunk = conn.recv(1024)
                    if not chunk:
                        break
                    request_bytes += chunk
                    if b"\r\n\r\n" in request_bytes:
                        break
                except OSError:
                    break

            request = request_bytes.decode('utf-8', 'ignore')
            if not request:
                conn.close()
                return

            lines = request.split("\r\n")
            if not lines:
                conn.close()
                return

            req_line = lines[0]
            parts = req_line.split(" ")
            if len(parts) < 2:
                conn.close()
                return

            method, path = parts[0], parts[1]

            if method == "GET" and path == "/status":
                self._handle_status(conn)
            elif method == "POST" and path == "/save":
                # Find body
                body = ""
                if "\r\n\r\n" in request:
                    body = request.split("\r\n\r\n", 1)[1]
                else:
                    # Try to read body if content-length is present
                    content_length = 0
                    for line in lines:
                        if line.lower().startswith("content-length:"):
                            content_length = int(line.split(":")[1].strip())
                    if content_length > 0:
                        body_bytes = conn.recv(content_length)
                        body = body_bytes.decode('utf-8', 'ignore')
                self._handle_save(conn, body)
            elif method == "GET" and (path == "/" or path == "/index.html"):
                self._handle_root(conn)
            else:
                conn.send("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n")
                conn.close()
        except Exception as e:
            print("WebServer: Error handling client:", e)
            try:
                conn.close()
            except:
                pass

    def _handle_root(self, conn):
        config = self.config_manager.get_config()

        html = "<!DOCTYPE html><html><head>"
        html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        html += "<title>CRSF Outputs & Web Monitor (Pico)</title>"
        html += "<style>"
        html += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; color: #333; text-align: center; padding: 20px; }"
        html += ".container { background-color: #fff; border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); display: inline-block; max-width: 500px; width: 100%; text-align: left; }"
        html += "h1 { color: #007bff; margin-bottom: 20px; font-size: 24px; text-align: center; }"
        html += "label { display: block; margin: 10px 0 5px; font-weight: bold; }"
        html += "select, input[type='number'] { width: 100%; padding: 10px; border-radius: 4px; border: 1px solid #ccc; font-size: 16px; margin-bottom: 15px; background: #fff; box-sizing: border-box; }"
        html += "input[type='submit'] { background-color: #007bff; color: #fff; border: none; padding: 12px 20px; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; font-weight: bold; }"
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
        html += "function updateStatus() {"
        html += "  fetch('/status').then(r => r.json()).then(data => {"
        html += "    document.getElementById('byteCount').innerText = data.bytes;"
        html += "    document.getElementById('packetCount').innerText = data.packets;"
        html += "    const link = document.getElementById('linkStatus');"
        html += "    if(data.bytes > 0 && data.packets > 0) {"
        html += "      link.innerText = 'ONLINE'; link.className = 'status-indicator status-ok';"
        html += "    } else {"
        html += "      link.innerText = 'NO DATA'; link.className = 'status-indicator';"
        html += "    }"
        html += "    for(let i=0; i<16; i++) {"
        html += "      const cell = document.getElementById('ch' + i);"
        html += "      if(cell) cell.innerText = 'CH' + (i+1) + ': ' + data.channels[i];"
        html += "    }"
        html += "  });"
        html += "}"
        html += "setInterval(updateStatus, 1000);"
        html += "</script>"
        html += "</head><body>"

        html += "<div class='container'>"
        html += "<h1>CRSF Web Config & Port Monitor (Pico)</h1>"

        # Live Serial Status Panel
        html += "<div class='panel'>"
        html += "<h3>Live Link Monitor</h3>"
        html += "<p>Link Status: <span id='linkStatus' class='status-indicator'>CONNECTING...</span></p>"
        html += "<p>Bytes Received: <span id='byteCount'>0</span></p>"
        html += "<p>Parsed Packets: <span id='packetCount'>0</span></p>"
        html += "<strong>Channel Values (ticks):</strong>"
        html += "<div class='grid'>"
        for i in range(16):
            html += "<div class='grid-item' id='ch" + str(i) + "'>CH" + str(i+1) + ": -</div>"
        html += "</div>"
        html += "</div>"

        # Configuration Form
        html += "<form action='/save' method='POST'>"

        # Switch Channel Dropdown & Limits
        html += "<h3>1. Switch Output</h3>"
        html += "<label for='switch'>CRSF Channel Map:</label>"
        html += "<select name='switch' id='switch'>"
        for i in range(1, 17):
            selected = "selected" if config["switchChannel"] == i else ""
            html += "<option value='" + str(i) + "' " + selected + ">CH " + str(i) + "</option>"
        html += "</select>"

        html += "<div class='row'>"
        html += "  <div><label>Min PWM (us):</label><input type='number' name='sw_min' value='" + str(config["switchMin"]) + "' min='500' max='2500'></div>"
        html += "  <div><label>Max PWM (us):</label><input type='number' name='sw_max' value='" + str(config["switchMax"]) + "' min='500' max='2500'></div>"
        html += "</div>"

        html += "<hr>"

        # Servo Channel Dropdown & Limits
        html += "<h3>2. Servo Output</h3>"
        html += "<label for='servo'>CRSF Channel Map:</label>"
        html += "<select name='servo' id='servo'>"
        for i in range(1, 17):
            selected = "selected" if config["servoChannel"] == i else ""
            html += "<option value='" + str(i) + "' " + selected + ">CH " + str(i) + "</option>"
        html += "</select>"

        html += "<div class='row'>"
        html += "  <div><label>Min PWM (us):</label><input type='number' name='srv_min' value='" + str(config["servoMin"]) + "' min='500' max='2500'></div>"
        html += "  <div><label>Max PWM (us):</label><input type='number' name='srv_max' value='" + str(config["servoMax"]) + "' min='500' max='2500'></div>"
        html += "</div>"

        html += "<hr>"

        # Camera Switch Channel Dropdown & Limits
        html += "<h3>3. Camera Switch Output</h3>"
        html += "<label for='camera'>CRSF Channel Map:</label>"
        html += "<select name='camera' id='camera'>"
        for i in range(1, 17):
            selected = "selected" if config["cameraChannel"] == i else ""
            html += "<option value='" + str(i) + "' " + selected + ">CH " + str(i) + "</option>"
        html += "</select>"

        html += "<div class='row'>"
        html += "  <div><label>Min PWM (us):</label><input type='number' name='cam_min' value='" + str(config["cameraMin"]) + "' min='500' max='2500'></div>"
        html += "  <div><label>Max PWM (us):</label><input type='number' name='cam_max' value='" + str(config["cameraMax"]) + "' min='500' max='2500'></div>"
        html += "</div>"

        html += "<hr>"

        # CRSF Baudrate Dropdown
        html += "<h3>4. System Configuration</h3>"
        html += "<label for='baud'>CRSF Receiver Baudrate:</label>"
        html += "<select name='baud' id='baud'>"
        bauds = [115200, 400000, 420000]
        for b in bauds:
            selected = "selected" if config["crsfBaudrate"] == b else ""
            html += "<option value='" + str(b) + "' " + selected + ">" + str(b) + " baud</option>"
        html += "</select>"

        html += "<input type='submit' value='Save & Apply Configuration'>"
        html += "</form>"
        html += "</div>"

        html += "</body></html>"

        conn.send("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n")
        conn.send(html)
        conn.close()

    def _parse_url_encoded(self, body):
        params = {}
        pairs = body.split('&')
        for pair in pairs:
            if '=' in pair:
                k, v = pair.split('=', 1)
                v = v.replace('+', ' ')
                # Simple loop-based %XX decoding for MicroPython compatibility
                if '%' in v:
                    chunks = v.split('%')
                    decoded = chunks[0]
                    for chunk in chunks[1:]:
                        if len(chunk) >= 2:
                            try:
                                decoded += chr(int(chunk[:2], 16)) + chunk[2:]
                            except ValueError:
                                decoded += '%' + chunk
                        else:
                            decoded += '%' + chunk
                    v = decoded
                params[k] = v
        return params

    def _handle_save(self, conn, body):
        params = self._parse_url_encoded(body)

        required_fields = ["switch", "servo", "camera", "baud", "sw_min", "sw_max", "srv_min", "srv_max", "cam_min", "cam_max"]
        if all(field in params for field in required_fields):
            try:
                new_config = {
                    "switchChannel": int(params["switch"]),
                    "servoChannel": int(params["servo"]),
                    "cameraChannel": int(params["camera"]),
                    "crsfBaudrate": int(params["baud"]),
                    "switchMin": int(params["sw_min"]),
                    "switchMax": int(params["sw_max"]),
                    "servoMin": int(params["srv_min"]),
                    "servoMax": int(params["srv_max"]),
                    "cameraMin": int(params["cam_min"]),
                    "cameraMax": int(params["cam_max"])
                }

                # Validation
                valid = (
                    1 <= new_config["switchChannel"] <= 16 and
                    1 <= new_config["servoChannel"] <= 16 and
                    1 <= new_config["cameraChannel"] <= 16 and
                    new_config["crsfBaudrate"] in [115200, 400000, 420000] and
                    500 <= new_config["switchMin"] <= 2500 and
                    500 <= new_config["switchMax"] <= 2500 and
                    500 <= new_config["servoMin"] <= 2500 and
                    500 <= new_config["servoMax"] <= 2500 and
                    500 <= new_config["cameraMin"] <= 2500 and
                    500 <= new_config["cameraMax"] <= 2500
                )

                if valid:
                    self.config_manager.save_config(new_config)

                    response = "<!DOCTYPE html><html><head>"
                    response += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                    response += "<title>Configuration Saved</title>"
                    response += "<style>"
                    response += "body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; padding: 50px; }"
                    response += ".message { background: #d4edda; color: #155724; padding: 20px; border-radius: 8px; border: 1px solid #c3e6cb; display: inline-block; max-width: 400px; text-align: left; }"
                    response += "a { display: inline-block; margin-top: 15px; color: #007bff; text-decoration: none; font-weight: bold; }"
                    response += "</style></head><body>"
                    response += "<div class='message'>"
                    response += "<h2>Configuration Saved Successfully!</h2>"
                    response += "<p><b>Switch:</b> CH" + str(new_config["switchChannel"]) + " (" + str(new_config["switchMin"]) + "us - " + str(new_config["switchMax"]) + "us)</p>"
                    response += "<p><b>Servo:</b> CH" + str(new_config["servoChannel"]) + " (" + str(new_config["servoMin"]) + "us - " + str(new_config["servoMax"]) + "us)</p>"
                    response += "<p><b>Camera:</b> CH" + str(new_config["cameraChannel"]) + " (" + str(new_config["cameraMin"]) + "us - " + str(new_config["cameraMax"]) + "us)</p>"
                    response += "<p><b>Baudrate:</b> " + str(new_config["crsfBaudrate"]) + " baud</p>"
                    response += "<p><strong>Note: Re-applying configuration requires restarting the device.</strong></p>"
                    response += "<a href='/'>&larr; Back to configuration</a>"
                    response += "</div></body></html>"

                    conn.send("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n")
                    conn.send(response)
                    conn.close()

                    # Graceful reboot of Pico W
                    print("Rebooting Pico to apply configuration...")
                    time.sleep(1.0)
                    import machine
                    machine.reset()
                    return
            except Exception as e:
                print("WebServer: Error during config save/reboot:", e)

        conn.send("HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n")
        conn.send("Invalid Parameters.")
        conn.close()

    def _handle_status(self, conn):
        channels_str = ",".join(str(self.parser.get_channel(i)) for i in range(16))
        json_payload = '{{"bytes": {}, "packets": {}, "channels": [{}]}}'.format(
            self.stats["bytes"], self.stats["packets"], channels_str
        )
        conn.send("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n")
        conn.send(json_payload)
        conn.close()
