# ESP32 CRSF to PWM Switch Controller

This project implements a CRSF (TBS Crossfire) protocol parser for the ESP32 that listens to **Channel 15** and converts it into a **PWM signal** (1000µs to 2000µs) to control an RC switch or servo.

Designed for the **Arduino IDE**, it uses hardware-optimized peripherals on the ESP32 for low latency and high precision.

## Features

- **Robust CRSF Parsing**: State-machine based parser with CRC8 validation (0xD5 polynomial).
- **11-bit Unpacking**: Full 16-channel extraction from CRSF Type 0x16 packets.
- **Hardware PWM**: Uses ESP32 `ledc` peripheral for 16-bit resolution 50Hz PWM output.
- **ESP32 Optimized**: Utilizes HardwareSerial (UART2) for incoming telemetry/RC data.

## Hardware Setup

| Component | ESP32 Pin | Note |
| --- | --- | --- |
| CRSF RX (From Receiver TX) | GPIO 16 | Hardware UART2 RX |
| CRSF TX (To Receiver RX) | GPIO 17 | Hardware UART2 TX (Optional for telemetry) |
| RC Switch PWM Output | GPIO 13 | Connect to signal pin of your RC switch |
| GND | GND | Common ground with Receiver and Switch |

## Installation (Arduino IDE)

1. Ensure you have the **ESP32 board support** installed in your Arduino IDE (Tools -> Board -> Boards Manager -> ESP32).
2. Download this project and ensure the directory is named `crsf_switch`.
3. Open `crsf_switch.ino` in the Arduino IDE.
4. Select your ESP32 board (e.g., "ESP32 Dev Module").
5. Upload to your board.

## Configuration

In `crsf_switch.ino`, you can adjust the following parameters:

- `BAUDRATE`: Set to `420000` (standard) or `115200` depending on your receiver settings.
- `TARGET_CHANNEL`: Default is `15`.
- `PWM_PIN`: Default is `13`.

## Technical Details

- **CRSF Range**: 172 (1000µs) to 1811 (2000µs). Center is 992 (1500µs).
- **PWM Frequency**: 50Hz (Standard for RC equipment).
- **PWM Resolution**: 16-bit for high precision pulse width generation.

## Project Structure

- `crsf_switch.ino`: Main Arduino sketch.
- `CrsfParser.h` / `.cpp`: Handles protocol synchronization and data extraction.
- `PwmController.h` / `.cpp`: Manages ESP32 hardware PWM timing.
