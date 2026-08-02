import json
import os

class ConfigManager:
    def __init__(self, filename="config.json"):
        self.filename = filename
        self.default_config = {
            "switchChannel": 15,
            "servoChannel": 16,
            "cameraChannel": 14,
            "crsfBaudrate": 400000,
            "switchMin": 1000,
            "switchMax": 2000,
            "servoMin": 1000,
            "servoMax": 2000,
            "cameraMin": 1000,
            "cameraMax": 2000
        }
        self.config = self.default_config.copy()

    def begin(self):
        try:
            # Check if file exists
            with open(self.filename, "r") as f:
                loaded = json.load(f)
                # Merge loaded configs with defaults in case of missing keys
                for k, v in self.default_config.items():
                    self.config[k] = loaded.get(k, v)
        except Exception as e:
            print("ConfigManager: File not found or corrupt, using defaults:", e)
            self.save_config(self.default_config)
            self.config = self.default_config.copy()

        self._validate_config()

    def _validate_config(self):
        # Bounds check channels (1-16)
        for key in ["switchChannel", "servoChannel", "cameraChannel"]:
            if not (1 <= self.config[key] <= 16):
                self.config[key] = self.default_config[key]

        # Bounds check baudrate
        if self.config["crsfBaudrate"] not in [115200, 400000, 420000]:
            self.config["crsfBaudrate"] = 400000

        # Bounds check PWM limits (500us - 2500us)
        for key in ["switchMin", "switchMax", "servoMin", "servoMax", "cameraMin", "cameraMax"]:
            if not (500 <= self.config[key] <= 2500):
                self.config[key] = self.default_config[key]

    def get_config(self):
        return self.config

    def save_config(self, new_config):
        self.config = new_config.copy()
        self._validate_config()
        try:
            with open(self.filename, "w") as f:
                json.dump(self.config, f)
            print("ConfigManager: Configuration saved successfully:", self.config)
        except Exception as e:
            print("ConfigManager: Failed to save config:", e)
