import time
from crsf_parser import CRSFParser
from pwm_control import PWMController

try:
    from machine import UART
except ImportError:
    # Mocking for non-ESP32 environments
    class UART:
        def __init__(self, id, baudrate, tx, rx):
            print(f"Mock UART {id} initialized at {baudrate}")
        def any(self):
            return 0
        def read(self, n):
            return None

# Configuration
UART_ID = 2
BAUDRATE = 420000 # Standard CRSF baudrate (can be 416666 or 420000)
TX_PIN = 17
RX_PIN = 16
PWM_PIN = 13 # Pin to control the RC switch
TARGET_CHANNEL = 15 # 1-indexed

def main():
    print("Starting CRSF to PWM Switch Controller...")

    # Initialize UART for CRSF
    # ESP32 UART2 is often used on pins 16(RX) and 17(TX)
    uart = UART(UART_ID, baudrate=BAUDRATE, tx=TX_PIN, rx=RX_PIN)

    parser = CRSFParser()
    controller = PWMController(pin_num=PWM_PIN, channel_index=TARGET_CHANNEL-1)

    while True:
        if uart.any():
            data = uart.read(uart.any())
            if data:
                for byte in data:
                    channels = parser.receive_byte(byte)
                    if channels:
                        pwm_val = controller.update_switch(channels)
                        if pwm_val:
                            # Optional: print debug info every second or so
                            # print(f"Ch15: {channels[14]} -> PWM: {pwm_val}us")
                            pass

        # Small sleep to yield to system (MicroPython specific)
        time.sleep_ms(1)

if __name__ == "__main__":
    main()
