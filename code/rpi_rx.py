import time
from LoRaRF import SX127x
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad

# ================= AES CONFIG =================
AES_KEY = bytes([
    0x2B, 0x7E, 0x15, 0x16,
    0x28, 0xAE, 0xD2, 0xA6,
    0xAB, 0xF7, 0x8C, 0x53,
    0x4A, 0xF1, 0xE1, 0x1A
])

AES_IV_MASTER = bytes([
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
])

BLOCK_SIZE = 16

# ================= LORA CONFIG =================
bus, cs, reset, dio0 = 0, 0, 17, 4
lora = SX127x()

if not lora.begin(bus, cs, reset, dio0):
    print("LoRa init failed")
    exit()

lora.setFrequency(433000000)

print("LoRa RX Ready")

# ================= RX STATE =================
encrypted_blocks = []
last_packet_time = 0
TIMEOUT = 2.0  # seconds

# ================= MAIN LOOP =================
while True:
    packet_size = lora.parsePacket()

    if packet_size == BLOCK_SIZE:
        block = bytes(lora.read(BLOCK_SIZE))
        encrypted_blocks.append(block)
        last_packet_time = time.time()

        print("Received HEX:", " ".join(f"{b:02X}" for b in block))

    # ---- End of message detected by timeout ----
    if encrypted_blocks and (time.time() - last_packet_time > TIMEOUT):
        try:
            # Reset IV per message
            cipher = AES.new(AES_KEY, AES.MODE_CBC, AES_IV_MASTER)

            ciphertext = b"".join(encrypted_blocks)
            plaintext_padded = cipher.decrypt(ciphertext)

            plaintext = unpad(plaintext_padded, BLOCK_SIZE)

            print("Decrypted Message:", plaintext.decode(errors="ignore"))
            print("")

        except ValueError:
            print("Invalid padding or decryption error\n")

        encrypted_blocks.clear()
