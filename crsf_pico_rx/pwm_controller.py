from machine import Pin, PWM
import time

class PwmController:
    CRSF_MIN = 172
    CRSF_MAX = 1811

    def __init__(self, switch_pin, servo_pin, camera_pin):
        self._switch_pin_num = switch_pin
        self._servo_pin_num = servo_pin
        self._camera_pin_num = camera_pin

        self._switch_pwm = None
        self._servo_pwm = None
        self._camera_pwm = None

    def begin(self):
        # RP2040 microsecond PWM frequency: 50Hz (20ms period)
        self._switch_pwm = PWM(Pin(self._switch_pin_num))
        self._switch_pwm.freq(50)

        self._servo_pwm = PWM(Pin(self._servo_pin_num))
        self._servo_pwm.freq(50)

        self._camera_pwm = PWM(Pin(self._camera_pin_num))
        self._camera_pwm.freq(50)

        print("PWM Outputs Initialized: Switch Pin {}, Servo Pin {}, Camera Pin {}".format(
            self._switch_pin_num, self._servo_pin_num, self._camera_pin_num
        ))

        # Sweeping test sequence at boot to verify physical connections
        print("Testing Outputs (Boot Sweep)...")
        self.write_micros(self._switch_pwm, 1000)
        self.write_micros(self._servo_pwm, 1000)
        self.write_micros(self._camera_pwm, 1000)
        time.sleep_ms(500)

        self.write_micros(self._switch_pwm, 1500)
        self.write_micros(self._servo_pwm, 1500)
        self.write_micros(self._camera_pwm, 1500)
        time.sleep_ms(500)

        self.write_micros(self._switch_pwm, 2000)
        self.write_micros(self._servo_pwm, 2000)
        self.write_micros(self._camera_pwm, 2000)
        time.sleep_ms(500)

        self.write_micros(self._switch_pwm, 1500)
        self.write_micros(self._servo_pwm, 1500)
        self.write_micros(self._camera_pwm, 1500)
        print("Outputs Ready.")

    def write_micros(self, pwm_instance, us):
        # machine.PWM on MicroPython uses duty_ns (nanoseconds) for ultra-high accuracy.
        # Microseconds to Nanoseconds: us * 1000
        pwm_instance.duty_ns(us * 1000)

    def _map(self, x, in_min, in_max, out_min, out_max):
        return int((x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min)

    def _constrain(self, x, min_val, max_val):
        return max(min_val, min(x, max_val))

    def update_switch(self, crsf_val, min_us, max_us):
        pwm_us = self._map(crsf_val, self.CRSF_MIN, self.CRSF_MAX, min_us, max_us)
        pwm_us = self._constrain(pwm_us, 500, 2500)
        self.write_micros(self._switch_pwm, pwm_us)

    def update_servo(self, crsf_val, min_us, max_us):
        pwm_us = self._map(crsf_val, self.CRSF_MIN, self.CRSF_MAX, min_us, max_us)
        pwm_us = self._constrain(pwm_us, 500, 2500)
        self.write_micros(self._servo_pwm, pwm_us)

    def update_camera(self, crsf_val, min_us, max_us):
        pwm_us = self._map(crsf_val, self.CRSF_MIN, self.CRSF_MAX, min_us, max_us)
        pwm_us = self._constrain(pwm_us, 500, 2500)
        self.write_micros(self._camera_pwm, pwm_us)
