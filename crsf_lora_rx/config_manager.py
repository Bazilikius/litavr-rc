import json
import os

class ConfigManager:
    DEFAULT_CONFIG = {
        "boxId": 1,
        "boxSelectChannel": 15,
        "servoChannel": 16,
        "servoTrigger": 1500,
        "servoInvertLeft": 0,
        "servoInvertRight": 0,
        "servoMin": 1500,
        "servoMax": 2000,
        "servoSpeed": 100,
        "loraFreq": 433000000,
        "allOneChannel": 0,
        "mosfetOffLevel": 0
    }

    def __init__(self, filename="rx_config.json"):
        self.filename = filename
        self.config = self.DEFAULT_CONFIG.copy()

    def load(self):
        try:
            # Check if file exists in MicroPython filesystem
            with open(self.filename, "r") as f:
                loaded = json.load(f)
                for k, v in loaded.items():
                    if k in self.DEFAULT_CONFIG:
                        self.config[k] = v
        except Exception as e:
            print("[Config] Failed to load config, using defaults:", e)
            self.save()
        return self.config

    def save(self, new_config=None):
        if new_config:
            self.config.update(new_config)
        if self.config.get("allOneChannel", 0):
            self.config["boxSelectChannel"] = self.config["servoChannel"]
        try:
            with open(self.filename, "w") as f:
                json.dump(self.config, f)
            print("[Config] Configuration successfully saved:", self.config)
        except Exception as e:
            print("[Config] Failed to save config:", e)
