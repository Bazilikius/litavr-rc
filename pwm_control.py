try:
    from machine import Pin, PWM
except ImportError:
    # Mocking for non-ESP32 environments
    class PWM:
        def __init__(self, pin, freq=50):
            self.pin = pin
            self.freq_val = freq
            print(f"Mock PWM initialized on pin {pin} with freq {freq}")
        def duty_ns(self, ns):
            print(f"Mock PWM duty_ns: {ns}ns")
        def freq(self, f):
            self.freq_val = f
    class Pin:
        def __init__(self, p):
            self.p = p

class PWMController:
    # CRSF values are typically 172 to 1811 (1000us to 2000us)
    CRSF_MIN = 172
    CRSF_MAX = 1811

    PWM_MIN = 1000 # us
    PWM_MAX = 2000 # us

    def __init__(self, pin_num, channel_index=14): # Channel 15 is index 14
        self.channel_index = channel_index
        # PWM frequency for RC servos/switches is usually 50Hz
        self.pwm = PWM(Pin(pin_num), freq=50)

    @staticmethod
    def crsf_to_pwm(crsf_value):
        """
        Maps CRSF channel value (172-1811) to PWM pulse width (1000-2000us).
        """
        pwm_us = PWMController.PWM_MIN + (crsf_value - PWMController.CRSF_MIN) * \
                 (PWMController.PWM_MAX - PWMController.PWM_MIN) / \
                 (PWMController.CRSF_MAX - PWMController.CRSF_MIN)

        return max(PWMController.PWM_MIN, min(PWMController.PWM_MAX, int(pwm_us)))

    def update_switch(self, channels):
        if channels and len(channels) > self.channel_index:
            crsf_val = channels[self.channel_index]
            pwm_us = self.crsf_to_pwm(crsf_val)
            # ESP32 MicroPython duty_ns takes nanoseconds
            self.pwm.duty_ns(pwm_us * 1000)
            return pwm_us
        return None
