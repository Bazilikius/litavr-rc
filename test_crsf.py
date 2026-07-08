from crsf_parser import CRSFParser
from pwm_control import PWMController

def test_crc():
    print("Testing CRC8...")
    parser = CRSFParser()
    # Example from spec (implied)
    # 0x16 type + payload (all zero)
    test_data = bytes([0x16] + [0]*22)
    crc = parser.crc8(test_data)
    print(f"CRC of type 0x16 + 22 zeros: {hex(crc)}")

def test_parsing():
    print("\nTesting Channel Parsing...")
    parser = CRSFParser()
    # 22 bytes of payload. 16 channels * 11 bits = 176 bits = 22 bytes.
    # Let's set Channel 15 to 1811 (max) and others to 992 (center).
    # Ch15 starts at bit index 14 * 11 = 154.

    channels = [992] * 16
    channels[14] = 1811 # Channel 15

    payload = bytearray(22)
    bits = 0
    for ch in channels:
        # Pack 11 bits of ch into payload
        for i in range(11):
            if ch & (1 << i):
                payload[bits // 8] |= (1 << (bits % 8))
            bits += 1

    # Construct frame: [Sync=0xC8] [Len=24] [Type=0x16] [Payload(22)] [CRC]
    frame_no_crc = bytes([0xC8, 24, 0x16]) + payload
    crc = parser.crc8(frame_no_crc[2:])
    full_frame = frame_no_crc + bytes([crc])

    # Feed to parser
    decoded = None
    for b in full_frame:
        decoded = parser.receive_byte(b)

    if decoded:
        print(f"Decoded Channel 15: {decoded[14]}")
        assert decoded[14] == 1811
        print("Success: Channel 15 matches!")
    else:
        print("Failure: Frame not decoded")

def test_pwm_mapping():
    print("\nTesting PWM Mapping...")
    # CRSF 172 -> 1000us
    # CRSF 992 -> 1500us
    # CRSF 1811 -> 2000us

    val_min = PWMController.crsf_to_pwm(172)
    val_mid = PWMController.crsf_to_pwm(992)
    val_max = PWMController.crsf_to_pwm(1811)

    print(f"172 -> {val_min}us")
    print(f"992 -> {val_mid}us")
    print(f"1811 -> {val_max}us")

    assert val_min == 1000
    assert abs(val_mid - 1500) <= 1
    assert val_max == 2000
    print("Success: PWM mapping accurate!")

if __name__ == "__main__":
    test_crc()
    test_parsing()
    test_pwm_mapping()
