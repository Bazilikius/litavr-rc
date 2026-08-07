# ESP32-C3 Super Mini CRSF Triple Output & Web Configurator

This project implements a CRSF (TBS Crossfire) protocol parser for the **ESP32-C3 Super Mini** that parses incoming RC channels, controls three independent PWM outputs (**RC Switch**, **Servo**, and **Camera Switch**), and provides a local **WiFi Web Interface** featuring a live status monitor and highly customizable PWM signal limits for each output.

## Hardware Setup

This project supports both **ESP32-C3 Super Mini** and classic **ESP32 Dev Module** hardware platforms. The code automatically detects the target platform at compile-time and configures the pin mapping accordingly.

### 1. ESP32-C3 Super Mini Pinout

| Component | Super Mini Pin | Arduino Pin | Note |
| --- | --- | --- | --- |
| CRSF RX (From Receiver TX) | GPIO 6 | 6 | Serial1 RX (Listen-only configuration) |
| CRSF TX (To Receiver RX) | - | - | Unused (TX set to -1 on ESP to prevent transmission) |
| RC Switch PWM Output | GPIO 5 | 5 | PWM Output (Configurable Channel & Limits) |
| Servo PWM Output | GPIO 4 | 4 | PWM Output (Configurable Channel & Limits) |
| Camera Switch PWM Output | GPIO 3 | 3 | PWM Output (Configurable Channel & Limits) |
| GND | GND | GND | Common Ground |
| 5V / VCC | 5V | 5V | Power for ESP32, Receiver, and Servos |

### 2. ESP32 Dev Module (Classic ESP32) Pinout

| Component | ESP32 Dev Pin | Arduino Pin | Note |
| --- | --- | --- | --- |
| CRSF RX (From Receiver TX) | GPIO 16 | 16 | Serial2 RX |
| CRSF TX (To Receiver RX) | GPIO 17 | 17 | Serial2 TX (Unused, Listen-only configuration) |
| RC Switch PWM Output | GPIO 5 | 5 | PWM Output (Configurable Channel & Limits) |
| Servo PWM Output | GPIO 4 | 4 | PWM Output (Configurable Channel & Limits) |
| Camera Switch PWM Output | GPIO 13 | 13 | PWM Output (Configurable Channel & Limits - Safe Pin) |
| GND | GND | GND | Common Ground |
| 5V / VCC | 5V / VIN | 5V / VIN | Power for ESP32, Receiver, and Servos |

## Features

- **WiFi Web Interface**: Configures channel mappings dynamically via a responsive webpage.
- **Customizable PWM Ranges**: Set individual Minimum and Maximum pulse widths (from 500µs up to 2500µs) for the Switch, Servo, and Camera Switch outputs. Perfect for adjusting servo travel endpoints, offsets, or switch travel!
- **Web Status / Port Monitor**: Live-updating browser view of incoming bytes, successfully parsed packets, and real-time tick values for all 16 CRSF channels.
- **Triple Outputs**: Map independent functions to Switch (GPIO 5), Servo (GPIO 4), and Camera Switch (GPIO 3).
- **Selectable Baudrates**: Supports 115200, 400000 (standard), and 420000 baud, configurable on-the-fly.
- **Persistent Preferences**: Saves customized parameters to non-volatile memory (NVS) using ESP32 Preferences.
- **Boot-up Sweeps**: Performs startup calibration/diagnostic sweeps on all three outputs to confirm connection.

## How to Configure Channel Mappings & PWM Limits

1. **Power on** the ESP32-C3 Super Mini. All three outputs will perform a 1.5-second test sweep to verify connections.
2. Search for WiFi networks on your phone or computer and connect to:
   - **SSID**: `CRSF-Config`
   - **Password**: `12345678`
3. Open your web browser and go to: **`http://192.168.4.1`**
4. The dashboard displays:
   - **Link Status** (ONLINE / NO DATA)
   - **Live Bytes Received** & **Parsed RC Packets**
   - **Live Channels Monitor** grid showing the current tick values of all 16 receiver channels.
5. In each output section, configure:
   - The mapped **CRSF Channel** (CH 1–16).
   - The **Minimum PWM (us)** and **Maximum PWM (us)** endpoints (default is 1000µs and 2000µs). This allows precise tuning of travel endpoints or endpoints limit for switches and servos!
6. Select the **Baudrate** matching your receiver. Click **Save Configuration** (the ESP32 will reboot automatically to safely apply your configuration changes).

## Installation (Arduino IDE)

1. Ensure you have the **ESP32 board support** installed (v3.0.0 or higher).
2. Select `ESP32C3 Dev Module` under boards.
3. In `Tools`, make sure **`USB CDC On Boot`** is set to **`Enabled`** to read the hardware debugging diagnostics in the Serial Monitor.
4. Disconnect RX (Pin 6) during uploading, then reconnect it.

## Troubleshooting

- **No Data in Web/Serial Monitor**: Ensure `USB CDC On Boot` is **Enabled** and check RX wiring.
- **Upload Fails**: Disconnect the receiver from Pin 6 (RX) before uploading. Incoming CRSF data can block the serial bootloader.
- **Outputs Not Moving**: Ensure a **Common Ground (GND)** is connected between the ESP32-C3, the receiver, and the servos/switches.
