import time
import network
from machine import Pin, reset
from config_manager import ConfigManager
from lora_module import LoraModule
from pwm_controller import PwmController
from web_server import WebServer

# Pin Definitions for ESP32 Dev Module
LEFT_SERVO_PIN = 4
RIGHT_SERVO_PIN = 13
MOSFET_PIN = 26       # External MOSFET
EXTRA_PIN = 15        # Power Key Pin with 60s delay
LED_PIN = 27          # Upper switch indicator LED
UPPER_SW_PIN = 32
LOWER_SW_PIN = 33
SERVO_UP_SW_PIN = 25  # 3rd switch (Servo UP)

LORA_SS = 5
LORA_RST = 14
LORA_DIO0 = 34        # Relocated to GPIO 34 to avoid flashing/boot conflict

# Debouncing state
debounced_upper = False
debounced_lower = False
debounced_servo_up = False

last_upper_time = 0
last_lower_time = 0
last_servo_up_time = 0

DEBOUNCE_DELAY_MS = 50

# Shared web server data
shared_data = {
    "packetCount": 0,
    "upperSw": False,
    "lowerSw": False,
    "servoUpSw": False,
    "override": False,
    "loweredTimestamp": 0,
    "channels": [1500] * 16
}

def is_trigger_active(val, trigger_pos):
    if trigger_pos == 1500:
        return (val >= 1400 and val <= 1600)
    else:
        # Tolerances of +/- 50 around extreme targets (950-1050 / 1950-2050)
        return (val >= (trigger_pos - 50) and val <= (trigger_pos + 50))

def get_selected_box(pw):
    if pw < 1000: pw = 1000
    if pw > 2000: pw = 2000
    box = ((pw - 1000) // 125) + 1
    if box < 1: box = 1
    if box > 8: box = 8
    return box

def main():
    print("\n=============================================")
    print(" MicroPython CRSF LoRa RX OUTPUT CONTROLLER ")
    print("=============================================")

    # Initialize switches
    upper_pin = Pin(UPPER_SW_PIN, Pin.IN, Pin.PULL_UP)
    lower_pin = Pin(LOWER_SW_PIN, Pin.IN, Pin.PULL_UP)
    servo_up_pin = Pin(SERVO_UP_SW_PIN, Pin.IN, Pin.PULL_UP)

    # 1. Load config
    cfg_manager = ConfigManager()
    cfg = cfg_manager.load()

    # 2. Init PWM controller
    controller = PwmController(
        LEFT_SERVO_PIN, RIGHT_SERVO_PIN, MOSFET_PIN, EXTRA_PIN,
        LED_PIN, UPPER_SW_PIN, LOWER_SW_PIN
    )

    # Startup diagnostic organic read
    is_upper_triggered = upper_pin.value() == 1
    controller.begin(
        cfg["servoInvertLeft"] != 0, cfg["servoInvertRight"] != 0,
        cfg["servoMin"], cfg["servoMax"], is_upper_triggered
    )

    # 3. Init LoRa SPI
    lora = LoraModule(LORA_SS, LORA_RST, LORA_DIO0)
    if not lora.begin(cfg["loraFreq"]):
        print("[LoRa] SX127x initialization failed! Checking connections...")
    else:
        lora.start_receive()

    # 4. Start AP & Web Server
    web = WebServer(cfg_manager, shared_data)
    web.start()

    boot_time = time.ticks_ms()
    wifi_shutdown_done = False

    global debounced_upper, debounced_lower, debounced_servo_up
    global last_upper_time, last_lower_time, last_servo_up_time

    # Primary main loop
    last_report_time = time.ticks_ms()
    print("[System] Main execution loop successfully started.")

    while True:
        # WiFi auto-shutdown after 3 minutes
        if not wifi_shutdown_done and time.ticks_diff(time.ticks_ms(), boot_time) > 180000:
            print("[System] 3 minutes elapsed. Disabling WiFi completely to conserve power.")
            ap = network.WLAN(network.AP_IF)
            ap.active(False)
            wifi_shutdown_done = True

        # Process Web clients
        if not wifi_shutdown_done:
            web.handle_client()

        # Parse LoRa packets
        p_size = lora.parse_packet()
        if p_size > 0:
            packet = lora.read_packet()
            # Verify packet signature: 0x55AA (or standard packet size matches)
            # Packet length of signature(2 bytes) + id(4 bytes) + 16 channels(32 bytes) = 38 bytes
            if len(packet) >= 38:
                sig = (packet[1] << 8) | packet[0]
                if sig == 0x55AA:
                    shared_data["packetCount"] += 1
                    # Extract channels (each is 2 bytes Little Endian)
                    for i in range(16):
                        offset = 6 + (i * 2)
                        raw_crsf = (packet[offset+1] << 8) | packet[offset]
                        # Scale to standard PWM range (988 - 2012)
                        shared_data["channels"][i] = ((raw_crsf - 992) * 5 // 8) + 1500

            lora.start_receive()

        # Read limit switches with stable debounce logic
        raw_u = upper_pin.value() == 1
        raw_l = lower_pin.value() == 1
        raw_su = servo_up_pin.value() == 1

        now = time.ticks_ms()

        if raw_u != debounced_upper:
            if time.ticks_diff(now, last_upper_time) > DEBOUNCE_DELAY_MS:
                debounced_upper = raw_u
                last_upper_time = now
        else:
            last_upper_time = now

        if raw_l != debounced_lower:
            if time.ticks_diff(now, last_lower_time) > DEBOUNCE_DELAY_MS:
                debounced_lower = raw_l
                last_lower_time = now
        else:
            last_lower_time = now

        if raw_su != debounced_servo_up:
            if time.ticks_diff(now, last_servo_up_time) > DEBOUNCE_DELAY_MS:
                debounced_servo_up = raw_su
                last_servo_up_time = now
        else:
            last_servo_up_time = now

        # Update indicators
        controller.led.value(1 if debounced_upper else 0)

        # Mesh Selection and Trigger evaluations
        box_sel_val = shared_data["channels"][cfg["boxSelectChannel"] - 1]
        active_box = get_selected_box(box_sel_val)
        is_selected = active_box == cfg["boxId"]

        trigger_active = is_trigger_active(shared_data["channels"][cfg["servoChannel"] - 1], cfg["servoTrigger"])

        servo_active = False
        mosfet_active = False

        is_any_open = debounced_upper or debounced_lower or debounced_servo_up
        shared_data["upperSw"] = debounced_upper
        shared_data["lowerSw"] = debounced_lower
        shared_data["servoUpSw"] = debounced_servo_up
        shared_data["override"] = is_any_open

        if is_selected:
            if trigger_active or debounced_servo_up:
                servo_active = True
                if not is_any_open:
                    mosfet_active = True

        # Update servos with linear speed-limiting
        controller.update_servos(
            servo_active, cfg["servoMin"], cfg["servoMax"],
            cfg["servoInvertLeft"] != 0, cfg["servoInvertRight"] != 0,
            cfg["servoSpeed"]
        )

        # 60s delay logic for Power Key (GPIO 15)
        all_three_closed = not is_any_open
        timer_enabled = is_selected and not servo_active and all_three_closed

        lowered_ts = shared_data["loweredTimestamp"]

        if timer_enabled:
            if lowered_ts == 0:
                shared_data["loweredTimestamp"] = time.ticks_ms()
        else:
            shared_data["loweredTimestamp"] = 0

        # Check if 60 seconds have elapsed
        elapsed_60s = False
        lowered_ts = shared_data["loweredTimestamp"]
        if lowered_ts != 0:
            if time.ticks_diff(time.ticks_ms(), lowered_ts) >= 60000:
                elapsed_60s = True

        extra_active = elapsed_60s and all_three_closed

        # Update PWM outputs
        controller.update_pwm_outputs(mosfet_active, extra_active, cfg["mosfetOffLevel"])

        # RED LED blinking during countdown, solid when active
        lowered_ts = shared_data["loweredTimestamp"]
        if lowered_ts != 0:
            if elapsed_60s:
                controller.red_led.value(1)
            else:
                blink = (time.ticks_ms() // 500) % 2 == 0
                controller.red_led.value(1 if blink else 0)
        else:
            controller.red_led.value(0)

        # BLUE LED blinking when servos are moving, solid when stationary
        moving = controller.is_servo_moving(
            servo_active, cfg["servoMin"], cfg["servoMax"],
            cfg["servoInvertLeft"] != 0, cfg["servoInvertRight"] != 0
        )
        if moving:
            blink = (time.ticks_ms() // 200) % 2 == 0
            controller.blue_led.value(1 if blink else 0)
        else:
            controller.blue_led.value(1)

        # Non-blocking yield
        time.sleep_ms(1)

        # 5-second diagnostics log
        if time.ticks_diff(time.ticks_ms(), last_report_time) > 5000:
            remaining = 60
            lowered_ts = shared_data["loweredTimestamp"]
            if lowered_ts != 0:
                remaining = max(0, 60 - (time.ticks_diff(time.ticks_ms(), lowered_ts) // 1000))
            print("[RX] Packets: %d | Box: %d (My: %d) | Upper: %s | Lower: %s | ServoUp: %s | Servos: %s | MOSFET: %s | Extra (60s timer remaining: %ds): %s" % (
                shared_data["packetCount"], active_box, cfg["boxId"],
                "OPEN" if debounced_upper else "CLOSED",
                "OPEN" if debounced_lower else "CLOSED",
                "OPEN" if debounced_servo_up else "CLOSED",
                "ACTIVE (2000us)" if servo_active else "NEUTRAL (1500us)",
                "ACTIVE (1500us)" if mosfet_active else "OFF",
                remaining,
                "ACTIVE (2000us)" if extra_active else "OFF"
            ))
            last_report_time = time.ticks_ms()

if __name__ == "__main__":
    main()
