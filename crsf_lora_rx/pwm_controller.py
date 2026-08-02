import time
from machine import Pin, PWM

class PwmController:
    def __init__(self, left_pin, right_pin, mosfet_pin, extra_pin, led_pin, upper_sw_pin, lower_sw_pin):
        self.left_pin = left_pin
        self.right_pin = right_pin
        self.mosfet_pin = mosfet_pin
        self.extra_pin = extra_pin
        self.led_pin = led_pin
        self.upper_sw_pin = upper_sw_pin
        self.lower_sw_pin = lower_sw_pin

        # Configure physical indicators and outputs
        self.led = Pin(led_pin, Pin.OUT)
        self.red_led = Pin(21, Pin.OUT)
        self.blue_led = Pin(22, Pin.OUT)

        self.red_led.value(1) # Red ON during boot
        self.blue_led.value(0)

        # Setup 50Hz PWM
        self.left_pwm = PWM(Pin(left_pin), freq=50)
        self.right_pwm = PWM(Pin(right_pin), freq=50)
        self.mosfet_pwm = PWM(Pin(mosfet_pin), freq=50)
        self.extra_pwm = PWM(Pin(extra_pin), freq=50)

        self.current_left_us = 1500.0
        self.current_right_us = 1500.0
        self.last_update_ms = time.ticks_ms()

    def begin(self, invert_left, invert_right, min_us, max_us, is_upper_triggered):
        if is_upper_triggered:
            self.current_left_us = float(min_us if invert_left else max_us)
            self.current_right_us = float(min_us if invert_right else max_us)
        else:
            self.current_left_us = float(max_us if invert_left else min_us)
            self.current_right_us = float(max_us if invert_right else min_us)

        # Self-test sweep on startup
        print("[PWM] Running boot-up diagnostics sweep...")
        self.write_micros(self.left_pwm, 1500)
        self.write_micros(self.right_pwm, 1500)
        time.sleep_ms(200)

        self.led.value(1)
        time.sleep_ms(300)
        self.led.value(0)

        self.red_led.value(0)
        self.blue_led.value(1) # Blue ON when stationary
        self.last_update_ms = time.ticks_ms()
        print("[PWM] Diagnostics complete. Outputs ready.")

    def update_servos(self, is_active, min_us, max_us, invert_left, invert_right, speed_us_per_sec):
        left_target = float(min_us if invert_left else max_us) if is_active else float(max_us if invert_left else min_us)
        right_target = float(min_us if invert_right else max_us) if is_active else float(max_us if invert_right else min_us)

        now = time.ticks_ms()
        elapsed = time.ticks_diff(now, self.last_update_ms)
        self.last_update_ms = now

        if elapsed > 0:
            max_step = float(elapsed) * (float(speed_us_per_sec) / 1000.0)

            # Slew rate limit Left Servo
            if self.current_left_us < left_target:
                self.current_left_us += max_step
                if self.current_left_us > left_target: self.current_left_us = left_target
            elif self.current_left_us > left_target:
                self.current_left_us -= max_step
                if self.current_left_us < left_target: self.current_left_us = left_target

            # Slew rate limit Right Servo
            if self.current_right_us < right_target:
                self.current_right_us += max_step
                if self.current_right_us > right_target: self.current_right_us = right_target
            elif self.current_right_us > right_target:
                self.current_right_us -= max_step
                if self.current_right_us < right_target: self.current_right_us = right_target

        self.write_micros(self.left_pwm, int(self.current_left_us))
        self.write_micros(self.right_pwm, int(self.current_right_us))

    def is_servo_moving(self, is_active, min_us, max_us, invert_left, invert_right):
        left_target = float(min_us if invert_left else max_us) if is_active else float(max_us if invert_left else min_us)
        right_target = float(min_us if invert_right else max_us) if is_active else float(max_us if invert_right else min_us)
        return (abs(self.current_left_us - left_target) > 1.0) or (abs(self.current_right_us - right_target) > 1.0)

    def update_pwm_outputs(self, is_mosfet_active, is_extra_active, mosfet_off_level):
        active_mosfet_us = 1500
        inactive_mosfet_us = 1000 if mosfet_off_level == 0 else 2000

        active_extra_us = 2000
        inactive_extra_us = 1000 if mosfet_off_level == 0 else 2000

        self.write_micros(self.mosfet_pwm, active_mosfet_us if is_mosfet_active else inactive_mosfet_us)
        self.write_micros(self.extra_pwm, active_extra_us if is_extra_active else inactive_extra_us)

    def write_micros(self, pwm_obj, us):
        # 50Hz corresponds to 20000 microseconds period.
        # MicroPython duty_u16 takes 0-65535.
        duty = int((us * 65535) / 20000)
        pwm_obj.duty_u16(duty)
