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
| **Left Servo PWM** | GPIO 4 | Output | Main left servo output (50Hz PWM) |
| **Right Servo PWM** | GPIO 13 | Output | Synchronous right servo output (Inversion supported) |
| **Extra Output Pin** | GPIO 26 | Output | Replicates active level of servos via 50Hz PWM for RC switches (HIGH = 2000us, LOW = 1000us) |
| **Power Key Output** | GPIO 15 | Output | Replicates delay logic level via 50Hz PWM for RC switches (HIGH = 2000us, LOW = 1000us) |
| **Status LED** | GPIO 27 | Output | Lights up when Upper limit switch is open/triggered |
| **Upper Limit Switch** | GPIO 32 | Input | Configured with `INPUT_PULLUP` |
| **Lower Limit Switch** | GPIO 33 | Input | Configured with `INPUT_PULLUP` |
| **Servo UP Limit Switch** | GPIO 25 | Input | Configured with `INPUT_PULLUP` |
| **LoRa SS/CS** | GPIO 5 | Output | SPI Slave Select for RFM95/RFM98W/SX1278 |
| **LoRa RST** | GPIO 14 | Output | Reset pin |
| **LoRa DIO0** | GPIO 2 | Input | Interrupt pin |
| **LoRa SCK** | GPIO 18 | Output | SPI SCK |
| **LoRa MISO** | GPIO 19 | Input | SPI MISO |
| **LoRa MOSI** | GPIO 23 | Output | SPI MOSI |

---

## 3. Advanced Features

1. **Synchronous Servo Control with Independent Inversions**:
   - Controls two physical servos (Left on GPIO 4, Right on GPIO 13) synchronously.
   - Independent software inversions are available in the Web Configurator for both Left and Right servos to ensure perfect mechanical coordination.
   - Startup sweep in `begin()` runs in perfect sync while respecting the user's Left and Right inversion parameters.

2. **Active-High Servo-UP Trigger Switch**:
   - A mechanical switch on GPIO 25 acts as a manual Servo-UP trigger.
   - When the switch is **open** (reads `HIGH`), it overrides normal control commands and drives both servos **UP** (active position, e.g. `maxUs`).
   - When the switch is **closed** (reads `LOW`), the servos operate in normal mode (either DOWN or controlled by the CRSF channel).

3. **Power Key Pin with 60-Second Delay & 3-Switch Safety Condition**:
   - Pin **GPIO 15** acts as a 5V Power Key to control external systems.
   - For safety, the Power Key activates (goes **HIGH**) only when:
     1. The servos have been **DOWN** (inactive) for at least **60 seconds** (tracked via the CRSF channel).
     2. **AND simultaneously**, ALL THREE mechanical switches are **CLOSED** (read `LOW` / connected to GND):
        * Upper Switch (GPIO 32) is closed (reads `LOW`)
        * Lower Switch (GPIO 33) is closed (reads `LOW`)
        * Servo-UP Switch (GPIO 25) is closed (reads `LOW`)
   - If any switch is opened (reads `HIGH`), the Power Key is instantly deactivated (goes **LOW**).

4. **Multi-Board Mesh/Addressing & Switch Tolerances**:
   - Broadcast allows up to 4 independent boards to listen to the same LoRa packet, but each board is configured to respond to a specific CRSF channel and switch trigger position (e.g., 1000, 1500, or 2000).
   - **Central Position (1500)**: Triggers inside `1400-1600` to compensate for joystick/switch slop.
   - **Edge Positions (1000 / 2000)**: Trigger inside a `±50` step range (e.g., `950-1050` for 1000; `1950-2050` for 2000).

5. **25% WiFi Tx Power Limitation**:
   - To conserve power, reduce heat, and prevent interference with the long-range LoRa telemetry link, the WiFi AP transmit power on both boards is limited to 25% (approx 5dBm).

---

## 4. Web Configurator Usage

1. **Power on** the RX ESP32 board.
2. Connect to the local WiFi Access Point:
   - **SSID**: `CRSF-Config-RX`
   - **Password**: `12345678`
3. Open your browser and go to: **`http://192.168.4.1`**
4. Configure the mapped channels, switch triggers, inversions, and custom PWM limits, then click **Save**. The board will save preferences to non-volatile storage and reboot.

---

## 5. Troubleshooting: Board Reboots on Limit Switch Trigger

If your ESP32 board reboots (resets) when both limit switches are triggered simultaneously, check the following potential hardware issues:

### 1. Incorrect Limit Switch Wiring (Short Circuit)
* **Problem**: The limit switches must be wired **strictly** between the GPIO pin and **GND**. If you connected the switch terminals to **VCC/3.3V/5V** and **GND** simultaneously, closing the switches creates a direct short circuit across the power rails, immediately causing a hardware reboot.
* **Solution**: Ensure each switch has only two connections: one terminal goes to the ESP32 GPIO, and the other terminal goes to a **GND** pin. *Never connect VCC or 3.3V/5V to the mechanical switches!*

### 2. Power Supply Brownouts (Servo Current Spikes)
* **Problem**: When both limit switches trigger, both servos instantly move to their neutral/safety positions at the same millisecond. Moving two servos simultaneously draws a massive transient current spike (1A to 2.5A). If you are powering the ESP32 and the servos from the same weak 5V line or via USB, the voltage will drop below 2.7V, triggering the ESP32's internal **Brownout Detector** which resets the CPU.
* **Solution**:
  * Add a large decoupling capacitor (at least **1000 µF, 6.3V or 10V**) across the 5V and GND rail close to the ESP32.
  * Connect the servos to a separate external power source (e.g., BEC or step-down converter) instead of drawing all current through the ESP32 Dev Module's regulator.

### 3. Inductive Back-EMF Spikes from the load
* **Problem**: Turning off high-load devices instantly creates a massive high-voltage spike (back-EMF) that enters the ESP32 ground plane, causing a CPU crash/reset.
* **Solution**: Always connect a flyback diode (e.g., **1N4007** or **1N5819**) in parallel with inductive loads (anode to GND, cathode to the positive load line) to absorb and dissipate the back-EMF spike.

---

## 6. Troubleshooting: Uploading / Flashing Failures on ESP32

If you encounter flashing errors such as:
`Warning: Failed to communicate with the flash chip, read/write operations will fail.`
`A fatal error occurred: Failed to connect to ESP32: No serial data received.`
`A fatal error occurred: Serial data stream stopped: Possible serial noise or corruption.`

This can be caused by two primary hardware factors:
1. **GPIO 12 (MTDI)** being pulled HIGH at boot, causing flash voltage conflicts. (Relocated to GPIO 4 in software to resolve).
2. **ESPTool failing to put the ESP32 into bootloader mode automatically** over the USB-to-UART bridge.

### Ultimate Flashing / Upload Recovery Checklist
If the console stays stuck on `Connecting....................` and ends with `No serial data received`, follow these steps to manually force the ESP32 into download mode:

1. **Unplug high-draw peripherals**: Temporarily unplug or disconnect your servos, receivers, or power keys from the ESP32 GPIO pins, as they can inject serial noise or draw too much current from the USB bus during flashing.
2. **Force Manual Bootloader Entry**:
   * Press and **HOLD** the physical **BOOT (or IO0) button** on your ESP32 board.
   * While holding the BOOT button, press and release the **EN (or RST) button** once.
   * Alternatively, just **HOLD** the **BOOT button** down while clicking **Upload** in the Arduino IDE.
   * Once you see the compiler finish and `Connecting...` appear, you can release the BOOT button. It will now connect and write to the flash chip flawlessly!
3. **Use a High-Quality USB Data Cable**: Ensure you are using a certified USB cable capable of data transfer, and try plugging it directly into a motherboard port (avoiding external unpowered hubs).
4. **Select 115200 Baud Rate**: If the upload gets corrupted or fails midway, change the `Upload Speed` in the `Tools` menu of Arduino IDE from `921600` to **`115200`** for maximum noise resistance.
