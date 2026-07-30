# ESP32 CRSF LoRa Multi-Board Remote Control System

This project splits the original passive receiver into a high-performance **CRSF-to-LoRa Transmitter (TX)** and a fleet of up to **4 Independent LoRa Receiver and Output Controller Boards (RX)** running a Mesh-like broadcast configuration over **ESP32 Dev Modules** and LoRa hardware.

---

## 1. System Architecture & Hardware Modules

```
                    +------------------------------------+
                    |        CRSF Transmitter            |
                    | (Ebyte E32-433T30D / SX1278 LoRa)  |
                    +-----------------+------------------+
                                      |
                            [LoRa 433 MHz / 3km+ Range]
                                      |
        +---------------------+-------+-------+---------------------+
        |                     |               |                     |
        v                     v               v                     v
  +-------------+       +-------------+ +-------------+       +-------------+
  |  RX Node 1  |       |  RX Node 2  | |  RX Node 3  |       |  RX Node 4  |
  | (RFM95/98W) |       | (RFM95/98W) | | (RFM95/98W) |       | (RFM95/98W) |
  +-------------+       +-------------+ +-------------+       +-------------+
```

### Radio Modules for 3km+ Long-Range Transmission:
* **Transmitter (TX) Module**: **Ebyte E32-433T30D** (1 Watt / 30dBm power output, 433MHz).
  * Pins **M0 (GND)** and **M1 (GND)** are connected to ground to configure the module in **Normal (Transparent Transmission) Mode**.
  * Driven via Serial interface to ensure high-power, reliable long-range broadcast.
* **Receiver (RX) Modules**: **RFM95 / RFM98W / SX1278** SPI LoRa chips.
  * Optimized with software settings for maximum sensitivity to reliably receive signals at **3km+ range** (Spreading Factor: **SF11**, Bandwidth: **125 kHz**, Coding Rate: **4/5**, with Low Data Rate Optimization active).

---

## 2. Hardware Wiring (ESP32 Dev Module)

### Transmitter Board (TX)
| Component / Pin | ESP32 Pin | Note |
| --- | --- | --- |
| **CRSF RX** | GPIO 16 (RX2) | Serial2 RX connection from TBS Crossfire / ELRS |
| **E32 TXD** | GPIO 27 | Serial1 RX (receives data from E32) |
| **E32 RXD** | GPIO 17 | Serial1 TX (sends parsed CRSF packets to E32) |
| **E32 M0** | GND | Hardwired to ground for transparent mode |
| **E32 M1** | GND | Hardwired to ground for transparent mode |
| **LoRa SS/CS** | GPIO 5 | SPI Slave Select (alternative built-in LoRa SPI) |
| **LoRa SCK** | GPIO 18 | SPI SCK |
| **LoRa MISO** | GPIO 19 | SPI MISO |
| **LoRa MOSI** | GPIO 23 | SPI MOSI |

> **Note for Ebyte E32-433T30D (TX)**: Because it outputs up to 1W (30dBm), it must be powered via an external 5V regulator capable of handling at least 1A peak current. Adding a 470uF decoupling capacitor across its VCC and GND pins is highly recommended to filter power spikes.

---

### Receiver Board (RX)
| Component / Pin | ESP32 Pin | Type | Note |
| --- | --- | --- | --- |
| **Left Servo PWM** | GPIO 12 | Output | Main left servo output (50Hz PWM) |
| **Right Servo PWM** | GPIO 13 | Output | Synchronous right servo output (Inversion supported) |
| **External MOSFET** | GPIO 25 | Output | Controlled via channel toggle & overridden by switches |
| **Extra Output Pin** | GPIO 26 | Output | Replicates MOSFET output level |
| **Status LED** | GPIO 27 | Output | Lights up when Upper limit switch is open |
| **Upper Limit Switch** | GPIO 32 | Input | Configured with `INPUT_PULLUP` (Open/Triggered reads HIGH) |
| **Lower Limit Switch** | GPIO 33 | Input | Configured with `INPUT_PULLUP` (Open/Triggered reads HIGH) |
| **LoRa SS/CS** | GPIO 5 | Output | SPI Slave Select for RFM95/RFM98W/SX1278 |
| **LoRa RST** | GPIO 14 | Output | Reset pin |
| **LoRa DIO0** | GPIO 2 | Input | Interrupt pin |
| **LoRa SCK** | GPIO 18 | Output | SPI SCK |
| **LoRa MISO** | GPIO 19 | Input | SPI MISO |
| **LoRa MOSI** | GPIO 23 | Output | SPI MOSI |

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
   - Broadcast allows up to 4 independent boards to listen to the same LoRa packet, but each board is configured to respond to a specific CRSF channel and switch trigger position (e.g., 1000, 1500, or 2000).
   - **Central Position (1500)**: Triggers inside `1400-1600` to compensate for joystick/switch slop.
   - **Edge Positions (1000 / 2000)**: Trigger inside a `±50` step range (e.g., `950-1050` for 1000; `1950-2050` for 2000).

4. **"All One Channel" Config ("Все одним каналом")**:
   - When enabled, the Servo configuration (channel and trigger position) is automatically duplicated to the MOSFET configuration. This simplifies set up to a single master channel.

5. **25% WiFi Tx Power Limitation**:
   - To conserve power, reduce heat, and prevent interference with the long-range LoRa telemetry link, the WiFi AP transmit power on both boards is limited to 25% (approx 5dBm).

---

## 4. Web Configurator Usage

1. **Power on** the RX ESP32 board.
2. Connect to the local WiFi Access Point:
   - **SSID**: `CRSF-Config-RX`
   - **Password**: `12345678`
3. Open your browser and go to: **`http://192.168.4.1`**
4. Configure the mapped channels, switch triggers, inversion, and custom PWM limits, then click **Save**. The board will save preferences to non-volatile storage and reboot.
