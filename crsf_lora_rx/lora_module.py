import time
from machine import Pin, SPI

class LoraModule:
    # Registers
    REG_FIFO = 0x00
    REG_OP_MODE = 0x01
    REG_FRF_MSB = 0x06
    REG_FRF_MID = 0x07
    REG_FRF_LSB = 0x08
    REG_PA_CONFIG = 0x09
    REG_FIFO_ADDR_PTR = 0x0d
    REG_FIFO_TX_BASE_ADDR = 0x0e
    REG_FIFO_RX_BASE_ADDR = 0x0f
    REG_FIFO_RX_CURRENT_ADDR = 0x10
    REG_IRQ_FLAGS = 0x12
    REG_RX_NB_BYTES = 0x13
    REG_MODEM_CONFIG_1 = 0x1d
    REG_MODEM_CONFIG_2 = 0x1e
    REG_PREAMBLE_MSB = 0x20
    REG_PREAMBLE_LSB = 0x21
    REG_PAYLOAD_LENGTH = 0x22
    REG_SYNC_WORD = 0x39
    REG_VERSION = 0x42

    MODE_LONG_RANGE_MODE = 0x80
    MODE_SLEEP = 0x00
    MODE_STANDBY = 0x01
    MODE_TX = 0x03
    MODE_RX_CONTINUOUS = 0x05

    IRQ_TX_DONE_MASK = 0x08
    IRQ_RX_DONE_MASK = 0x40
    IRQ_PAYLOAD_CRC_ERROR_MASK = 0x20

    def __init__(self, ss_pin, rst_pin, dio0_pin):
        self.ss = Pin(ss_pin, Pin.OUT)
        self.rst = Pin(rst_pin, Pin.OUT) if rst_pin is not None else None
        self.dio0 = Pin(dio0_pin, Pin.IN)
        self.ss.value(1)
        self.spi = None

    def begin(self, frequency, sck=18, miso=19, mosi=23):
        if self.rst:
            self.rst.value(1)
            time.sleep_ms(10)
            self.rst.value(0)
            time.sleep_ms(10)
            self.rst.value(1)
            time.sleep_ms(10)

        # SPIbaud rate up to 10MHz
        self.spi = SPI(1, baudrate=10000000, polarity=0, phase=0, sck=Pin(sck), mosi=Pin(mosi), miso=Pin(miso))

        version = self.read_register(self.REG_VERSION)
        if version != 0x12:
            print("[LoRa] SX127x version mismatch:", hex(version))
            return False

        self.sleep()
        self.set_frequency(frequency)

        # SF7, 125kHz bandwidth, explicit headers
        self.write_register(self.REG_MODEM_CONFIG_1, 0x72)
        self.write_register(self.REG_MODEM_CONFIG_2, 0x74)

        # Preamble 8
        self.write_register(self.REG_PREAMBLE_MSB, 0)
        self.write_register(self.REG_PREAMBLE_LSB, 8)

        # Sync Word
        self.write_register(self.REG_SYNC_WORD, 0x12)

        # PA Boost MAX
        self.write_register(self.REG_PA_CONFIG, 0xFF)

        # FIFO Bases
        self.write_register(self.REG_FIFO_TX_BASE_ADDR, 0)
        self.write_register(self.REG_FIFO_RX_BASE_ADDR, 0)

        self.idle()
        print("[LoRa] SX127x successfully initialized at", frequency, "Hz")
        return True

    def sleep(self):
        self.write_register(self.REG_OP_MODE, self.MODE_LONG_RANGE_MODE | self.MODE_SLEEP)

    def idle(self):
        self.write_register(self.REG_OP_MODE, self.MODE_LONG_RANGE_MODE | self.MODE_STANDBY)

    def set_frequency(self, freq):
        frf = int((freq << 19) / 32000000)
        self.write_register(self.REG_FRF_MSB, (frf >> 16) & 0xFF)
        self.write_register(self.REG_FRF_MID, (frf >> 8) & 0xFF)
        self.write_register(self.REG_FRF_LSB, frf & 0xFF)

    def start_receive(self):
        self.write_register(self.REG_OP_MODE, self.MODE_LONG_RANGE_MODE | self.MODE_RX_CONTINUOUS)

    def parse_packet(self):
        irq = self.read_register(self.REG_IRQ_FLAGS)
        self.write_register(self.REG_IRQ_FLAGS, irq) # Clear IRQs

        if (irq & self.IRQ_RX_DONE_MASK) and not (irq & self.IRQ_PAYLOAD_CRC_ERROR_MASK):
            return self.read_register(self.REG_RX_NB_BYTES)
        return 0

    def read_packet(self, max_len=64):
        length = self.read_register(self.REG_RX_NB_BYTES)
        if length > max_len:
            length = max_len

        curr_addr = self.read_register(self.REG_FIFO_RX_CURRENT_ADDR)
        self.write_register(self.REG_FIFO_ADDR_PTR, curr_addr)

        self.ss.value(0)
        self.spi.write(bytes([self.REG_FIFO & 0x7F]))
        buf = self.spi.read(length)
        self.ss.value(1)
        return buf

    def send_packet(self, buf):
        self.idle()
        self.write_register(self.REG_FIFO_ADDR_PTR, 0)
        self.write_register(self.REG_PAYLOAD_LENGTH, len(buf))

        self.ss.value(0)
        self.spi.write(bytes([self.REG_FIFO | 0x80]))
        self.spi.write(buf)
        self.ss.value(1)

        self.write_register(self.REG_OP_MODE, self.MODE_LONG_RANGE_MODE | self.MODE_TX)

        start = time.ticks_ms()
        while (self.read_register(self.REG_IRQ_FLAGS) & self.IRQ_TX_DONE_MASK) == 0:
            if time.ticks_diff(time.ticks_ms(), start) > 100:
                self.idle()
                return False
            time.sleep_us(100)

        self.write_register(self.REG_IRQ_FLAGS, self.IRQ_TX_DONE_MASK)
        self.idle()
        return True

    def read_register(self, reg):
        self.ss.value(0)
        self.spi.write(bytes([reg & 0x7F]))
        val = self.spi.read(1)[0]
        self.ss.value(1)
        return val

    def write_register(self, reg, val):
        self.ss.value(0)
        self.spi.write(bytes([reg | 0x80, val]))
        self.ss.value(1)
