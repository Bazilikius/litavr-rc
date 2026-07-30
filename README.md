# ESP32 CRSF LoRa Multi-Board Remote Control System

This project splits the original passive receiver into a high-performance **CRSF-to-LoRa Transmitter (TX)** and a fleet of up to **4 Independent LoRa Receiver and Output Controller Boards (RX)** running a Mesh-like broadcast configuration over **ESP32 Dev Modules** and **SX127x LoRa** modules.

---

## 1. System Architecture

```
                    +-----------------------------+
                    |      CRSF Transmitter       |
                    | (Listens to CRSF on Serial2)|
                    +--------------+--------------+
                                   |
                         [LoRa 433/868/915 MHz]
                     (Broadcasts 16 channel values)
                                   |
        +------------------+-------+--------+------------------+
        |                  |                |                  |
        v                  v                v                  v
  +-----------+      +-----------+    +-----------+      +-----------+
  |  RX Board |      |  RX Board |    |  RX Board |      |  RX Board |
  |  Node 1   |      |  Node 2   |    |  Node 3   |      |  Node 4   |
  +-----------+      +-----------+    +-----------+      +-----------+
```

---

## 2. Hardware Wiring (ESP32 Dev Module)

### Transmitter Board (TX)
| Component | ESP32 Pin | Note |
| --- | --- | --- |
| CRSF RX | GPIO 16 (RX2) | Serial2 RX connection to TBS Crossfire / ELRS |
| LoRa SS/CS | GPIO 5 | SPI Slave Select |
| LoRa RST | GPIO 14 | Reset pin |
| LoRa DIO0 | GPIO 2 | Interrupt / Rx-Tx-Done pin |
| LoRa SCK | GPIO 18 | SPI Clock |
| LoRa MISO | GPIO 19 | SPI Master In Slave Out |
| LoRa MOSI | GPIO 23 | SPI Master Out Slave In |

### Receiver Board (RX)
| Component | ESP32 Pin | Type | Note |
| --- | --- | --- | --- |
| **Left Servo PWM** | GPIO 12 | Output | Main left servo output (50Hz PWM) |
| **Right Servo PWM** | GPIO 13 | Output | Synchronous right servo output (Inversion supported) |
| **External MOSFET** | GPIO 25 | Output | Controlled via channel toggle & overridden by switches |
| **Extra Output Pin** | GPIO 26 | Output | Replicates MOSFET output level |
| **Status LED** | GPIO 27 | Output | Lights up when Upper limit switch is open |
| **Upper Limit Switch** | GPIO 32 | Input | Configured with `INPUT_PULLUP` (Open/Triggered reads HIGH) |
| **Lower Limit Switch** | GPIO 33 | Input | Configured with `INPUT_PULLUP` (Open/Triggered reads HIGH) |
| LoRa SS/CS | GPIO 5 | Output | SPI Slave Select |
| LoRa RST | GPIO 14 | Output | Reset pin |
| LoRa DIO0 | GPIO 2 | Input | Interrupt pin |
| LoRa SCK | GPIO 18 | Output | SPI Clock |
| LoRa MISO | GPIO 19 | Input | SPI MISO |
| LoRa MOSI | GPIO 23 | Output | SPI MOSI |

---

## 3. Advanced Features

1. **Synchronous Servo Control with Inversion**:
   - Controls two physical servos (Left on GPIO 12, Right on GPIO 13) synchronously.
   - An inversion toggle is available in the Web Configurator for the Right Servo, allowing opposite mechanical motion directions.

2. **Dual Limit Switches Safety & MOSFET Override**:
   - Monitor two physical switches (Upper and Lower) with internal pull-ups.
   - If **at least one** switch is open (reads `HIGH`), the system instantly blocks power by driving the MOSFET (GPIO 25) and Extra Pin (GPIO 26) to the user-configured **Off Level** (HIGH or LOW).
   - An LED (GPIO 27) automatically lights up if the **Upper** limit switch is open.

3. **Multi-Board Mesh/Addressing & Switch Tolerances**:
   - Broadcast allows 4 independent boards to listen to the same LoRa packet, but each board is configured to respond to a specific CRSF channel and switch trigger position (e.g., 1000, 1500, or 2000).
   - **Central Position (1500)**: Triggers inside `1400-1600` to compensate for joystick/switch slop.
   - **Edge Positions (1000 / 2000)**: Trigger inside a `±50` step range (e.g., `950-1050` for 1000; `1950-2050` for 2000).

4. **"All One Channel" Config ("Все одним каналом")**:
   - When enabled, the Servo configuration (channel and trigger position) is automatically duplicated to the MOSFET configuration. This simplifies set up to a single master channel.

5. **25% WiFi Tx Power Limitation**:
   - To conserve power, reduce heat, and prevent interference, the WiFi AP transmit power on both boards is limited to 25% (approx 5dBm).

---

## 4. Web Configurator Usage

1. **Power on** the RX ESP32 board.
2. Connect to the local WiFi Access Point:
   - **SSID**: `CRSF-Config-RX`
   - **Password**: `12345678`
3. Open your browser and go to: **`http://192.168.4.1`**
4. Configure the mapped channels, switch triggers, inversion, and custom PWM limits, then click **Save**. The board will save preferences to non-volatile storage and reboot.
