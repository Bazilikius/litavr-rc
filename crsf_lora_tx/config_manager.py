import json

class ConfigManager:
    DEFAULT_CONFIG = {
        "crsfBaudrate": 400000,
        "loraFreq": 433000000
    }

    def __init__(self, filename="tx_config.json"):
        self.filename = filename
        self.config = self.DEFAULT_CONFIG.copy()

    def load(self):
        try:
            with open(self.filename, "r") as f:
                loaded = json.load(f)
                for k, v in loaded.items():
                    if k in self.DEFAULT_CONFIG:
                        self.config[k] = v
        except Exception as e:
            print("[Config] Failed to load transmitter config, using defaults:", e)
            self.save()
        return self.config

    def save(self, new_config=None):
        if new_config:
            self.config.update(new_config)
        try:
            with open(self.filename, "w") as f:
                json.dump(self.config, f)
            print("[Config] Transmitter config saved:", self.config)
        except Exception as e:
            print("[Config] Failed to save transmitter config:", e)
