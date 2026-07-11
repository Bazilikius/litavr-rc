# ESP32-C3 Super Mini CRSF to PWM Switch Controller

This project implements a CRSF (TBS Crossfire) protocol parser for the **ESP32-C3 Super Mini** that listens to **Channel 15** and converts it into a **PWM signal** (1000µs to 2000µs) to control an RC switch.

## Hardware Setup (ESP32-C3 Super Mini)

| Component | Super Mini Pin | Arduino Pin | Note |
| --- | --- | --- | --- |
| CRSF RX (From Receiver TX) | GPIO 6 | 6 | Serial1 RX |
| CRSF TX (To Receiver RX) | GPIO 7 | 7 | Serial1 TX |
| RC Switch PWM Output | GPIO 10 | 10 | PWM Signal |
| GND | GND | GND | Common Ground |
| 5V / VCC | 5V | 5V | Power for ESP32 and Receiver |

## Installation (Arduino IDE)

1. **Board Support**: Install `esp32` by Espressif Systems (v3.0.0 or higher recommended).
2. **Board Selection**: Select `ESP32C3 Dev Module`.
3. **USB Mode**: Set `USB CDC On Boot` to `Enabled` to see Serial output.
4. **Flash**: Click Upload. If it fails, hold the `BOOT` button while plugging in the USB.

## Features

- **Optimized for C3**: Uses `Serial1` for CRSF data.
- **New LEDC API**: Compatible with Arduino ESP32 Core 3.0+.
- **400k Baudrate**: Pre-configured for standard high-speed CRSF.
- **Target Channel**: Specifically extracts Channel 15.

## Troubleshooting

- **No Data in Serial Monitor**: In Arduino IDE, ensure `USB CDC On Boot` is **Enabled**.
- **Upload Fails**: Disconnect the receiver from Pin 6 (RX) before uploading. Incoming CRSF data can block the serial bootloader.
- **No PWM**: Ensure you have a common ground between the ESP32, the RC Receiver, and the RC Switch.
- **Pin Mapping**: On the ESP32-C3 Super Mini, GPIO 2 is a strapping pin. We use **GPIO 6** and **GPIO 7** for CRSF to avoid boot issues.
