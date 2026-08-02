from machine import UART, Pin
import time
from config_manager import ConfigManager
from crsf_parser import CrsfParser
from pwm_controller import PwmController
from web_server import WebServer

# Hardware Pin Configuration (Raspberry Pi Pico)
# UART0 TX on GP0, RX on GP1 (Default UART pins for Raspberry Pi Pico)
UART_PORT = 0
RX_PIN = 1
TX_PIN = 0 # Read-only, but Pico machine.UART requires defining a pin or TX defaults to GP0

# PWM Outputs
SWITCH_PIN = 5
SERVO_PIN = 4
CAMERA_PIN = 3

def main():
    print("\n=============================================")
    print(" RASPBERRY PI PICO CRSF TRIPLE OUTPUT ")
    print("=============================================")
    print("Configured Pins: RX=GP{}, TX=GP{} (read-only), Switch=GP{}, Servo=GP{}, Camera=GP{}".format(
        RX_PIN, TX_PIN, SWITCH_PIN, SERVO_PIN, CAMERA_PIN
    ))

    # Initialize Config Manager
    config_manager = ConfigManager()
    config_manager.begin()
    config = config_manager.get_config()

    # Shared diagnostics statistics
    stats = {
        "bytes": 0,
        "packets": 0
    }

    # Initialize CRSF parser
    parser = CrsfParser()

    # Initialize PWM Controller
    controller = PwmController(SWITCH_PIN, SERVO_PIN, CAMERA_PIN)
    controller.begin()

    # Initialize WiFi AP & Web server (Pico W specific)
    web_server = WebServer(config_manager, parser, stats)
    web_server.begin()

    # Initialize UART for CRSF
    print("Initializing CRSF on UART{} at {} baud...".format(UART_PORT, config["crsfBaudrate"]))
    # rxBuf size is set to prevent packet drops during Web Server activity
    uart = UART(UART_PORT, baudrate=config["crsfBaudrate"], tx=Pin(TX_PIN), rx=Pin(RX_PIN), rxbuf=1024)

    last_report = time.ticks_ms()
    last_switch_val = 0
    last_servo_val = 0
    last_camera_val = 0

    while True:
        # 1. Handle background web server tasks (non-blocking)
        web_server.handle_client()

        # 2. Read and parse incoming serial byte-stream
        read_limit = 0
        while uart.any() and read_limit < 128:
            byte_data = uart.read(1)
            if not byte_data:
                break

            b = byte_data[0]
            stats["bytes"] += 1
            read_limit += 1

            if parser.process_byte(b):
                stats["packets"] += 1

                # Retrieve the active config values
                active_config = config_manager.get_config()

                sw_val = parser.get_channel(active_config["switchChannel"] - 1)
                srv_val = parser.get_channel(active_config["servoChannel"] - 1)
                cam_val = parser.get_channel(active_config["cameraChannel"] - 1)

                # Update physical PWM duty-cycle
                controller.update_switch(sw_val, active_config["switchMin"], active_config["switchMax"])
                controller.update_servo(srv_val, active_config["servoMin"], active_config["servoMax"])
                controller.update_camera(cam_val, active_config["cameraMin"], active_config["cameraMax"])

                # Log value changes to terminal
                if (abs(int(sw_val) - int(last_switch_val)) > 10 or
                    abs(int(srv_val) - int(last_servo_val)) > 10 or
                    abs(int(cam_val) - int(last_camera_val)) > 10):

                    print("Live CRSF Update -> Switch(Ch{}): {}, Servo(Ch{}): {}, Camera(Ch{}): {}".format(
                        active_config["switchChannel"], sw_val,
                        active_config["servoChannel"], srv_val,
                        active_config["cameraChannel"], cam_val
                    ))

                    last_switch_val = sw_val
                    last_servo_val = srv_val
                    last_camera_val = cam_val

        # 3. Handle yield to background tasks / prevent high-CPU loops
        time.sleep_ms(1)

        # 4. Diagnostics report every 5 seconds
        if time.ticks_diff(time.ticks_ms(), last_report) > 5000:
            if stats["bytes"] == 0:
                print("HARDWARE SERIAL MONITOR: No CRSF data received on GP{}. Check wiring.".format(RX_PIN))
            else:
                active_config = config_manager.get_config()
                print("HARDWARE SERIAL MONITOR: Bytes: {} | RC Packets: {} | Sw(Ch{}): {} | Srv(Ch{}): {} | Cam(Ch{}): {}".format(
                    stats["bytes"], stats["packets"],
                    active_config["switchChannel"], parser.get_channel(active_config["switchChannel"] - 1),
                    active_config["servoChannel"], parser.get_channel(active_config["servoChannel"] - 1),
                    active_config["cameraChannel"], parser.get_channel(active_config["cameraChannel"] - 1)
                ))
            last_report = time.ticks_ms()

if __name__ == "__main__":
    main()
