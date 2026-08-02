import time
import network
import struct
from machine import Pin, UART
from config_manager import ConfigManager
from crsf_parser import CrsfParser
from lora_module import LoraModule

# Hardware configuration
CRSF_RX_PIN = 16
CRSF_TX_PIN = 17

LORA_SS = 5
LORA_RST = 14
LORA_DIO0 = 34 # Safety-relocated to GPIO 34

def main():
    print("\n=============================================")
    print(" MicroPython CRSF to LoRa TRANSMITTER ")
    print("=============================================")

    # 1. Load config
    cfg_manager = ConfigManager()
    cfg = cfg_manager.load()

    # 2. Limit WiFi TX power to 25% ( txpower=8 )
    ap = network.WLAN(network.AP_IF)
    ap.active(True)
    ap.config(essid="CRSF-TX-Config", password="12345678")
    try:
        ap.config(txpower=8)
        print("[System] WiFi SoftAP started at 25% power.")
    except Exception as e:
        print("[System] Could not restrict WiFi TX Power:", e)

    # 3. Init LoRa SPI
    lora = LoraModule(LORA_SS, LORA_RST, LORA_DIO0)
    if not lora.begin(cfg["loraFreq"]):
        print("[LoRa] SX127x initialization failed! Checking connections...")
    else:
        print("[LoRa] SX127x initialized successfully.")

    # 4. Init UART2 for CRSF parsing
    print("[System] Initializing CRSF UART2 at %d baud (RX=%d)..." % (cfg["crsfBaudrate"], CRSF_RX_PIN))
    # Standard ESP32 UART2
    uart = UART(2, baudrate=cfg["crsfBaudrate"], rx=CRSF_RX_PIN, tx=CRSF_TX_PIN, bits=8, parity=None, stop=1)

    parser = CrsfParser()
    packet_id = 0
    lora_sent = 0
    byte_count = 0

    last_report = time.ticks_ms()

    while True:
        # Check UART bytes
        if uart.any():
            read_limit = 0
            # Cap reading to 128 bytes per cycle to prevent starvation
            while uart.any() and read_limit < 128:
                b = uart.read(1)
                if b:
                    byte_count += 1
                    read_limit += 1
                    if parser.process_byte(b[0]):
                        # Valid packet parsed, prepare LoRa payload
                        packet_id = (packet_id + 1) & 0xFFFFFFFF

                        # signature (uint16) + packet_id (uint32) + 16 channels (uint16)
                        # Payload format: '<H I 16H' -> 2 + 4 + 32 = 38 bytes
                        payload = struct.pack('<H I 16H', 0x55AA, packet_id, *parser.channels)

                        if lora.send_packet(payload):
                            lora_sent += 1

        # Non-blocking yield
        time.sleep_ms(1)

        # 5-second diagnostics report
        if time.ticks_diff(time.ticks_ms(), last_report) > 5000:
            print("[TX] Bytes read: %d | CRSF parsed: %d | LoRa Sent: %d" % (byte_count, packet_id, lora_sent))
            last_report = time.ticks_ms()

if __name__ == "__main__":
    main()
