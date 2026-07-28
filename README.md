# ESP32-C3 Super Mini CRSF Dual Output & Web Configurator

This project implements a CRSF (TBS Crossfire) protocol parser for the **ESP32-C3 Super Mini** that parses incoming RC channels, controls two independent PWM outputs (**RC Switch** and **Servo**), and provides a local **WiFi Web Interface** to configure channel mappings dynamically.

## Hardware Setup (ESP32-C3 Super Mini)

| Component | Super Mini Pin | Arduino Pin | Note |
| --- | --- | --- | --- |
| CRSF RX (From Receiver TX) | GPIO 6 | 6 | Serial1 RX |
| CRSF TX (To Receiver RX) | GPIO 7 | 7 | Serial1 TX |
| RC Switch PWM Output | GPIO 5 | 5 | PWM Output (Configurable Channel) |
| Servo PWM Output | GPIO 4 | 4 | PWM Output (Configurable Channel) |
| GND | GND | GND | Common Ground |
| 5V / VCC | 5V | 5V | Power for ESP32 and Receiver |

## Features

- **WiFi Web Interface**: Configures mappings dynamically via a responsive webpage.
- **Dual Outputs**: Customizes mapping for one RC Switch (GPIO 5) and one Servo (GPIO 4) to any of the 16 CRSF channels.
- **Persistent Preferences**: Saves your customized mappings directly to the ESP32's non-volatile memory.
- **Boot-up Sweeps**: Performs diagnostic startup sweeps on both outputs to confirm hardware connections.
- **Sliding-Window Parser**: Features solid synchronization to parse 400k baud data flawlessly even with noise.

## How to Configure Channel Mappings

1. **Power on** the ESP32-C3 Super Mini. Both outputs (Switch and Servo) will perform a 1.5-second test sweep to verify their electrical connections.
2. Search for WiFi networks on your phone or computer and connect to:
   - **SSID**: `CRSF-Config`
   - **Password**: `12345678`
3. Open your web browser and go to: **`http://192.168.4.1`**
4. Select the desired CRSF channels for the **Switch** and **Servo** outputs, and click **Save Configuration**.
5. The ESP32 will immediately store these preferences and apply the changes!

## Installation (Arduino IDE)

1. Ensure you have the **ESP32 board support** installed (v3.0.0 or higher recommended).
2. Select `ESP32C3 Dev Module` under boards.
3. In `Tools`, make sure **`USB CDC On Boot`** is set to **`Enabled`** to read the debugging diagnostics in the Serial Monitor.
4. Keep the RX pin disconnected during uploading, then reconnect it.

## Troubleshooting

- **No Data in Serial Monitor**: Ensure `USB CDC On Boot` is **Enabled**.
- **Upload Fails**: Disconnect the receiver from Pin 6 (RX) before uploading. Incoming CRSF data can block the serial bootloader.
- **Outputs Not Moving**: Ensure a **Common Ground (GND)** is connected between the ESP32-C3, the RC receiver, and the servo/RC switch.
