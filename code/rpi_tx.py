import time
from LoRaRF import SX127x
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

# 1. Matching Arduino configuration exactly
# Use bytes instead of char to avoid signed/unsigned issues
AES_KEY = bytes([0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                  0xAB, 0xF7, 0x8C, 0x53, 0x4A, 0xF1, 0xE1, 0x1A])
AES_IV_MASTER = bytes([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F])

# Pin configuration (RPI 4)
bus, cs, reset, dio0 = 0, 0, 17, 4
lora = SX127x()

if not lora.begin(bus, cs, reset, dio0):
    print("LoRa initialization failed!")
    exit()

lora.setFrequency(433000000)

def encrypt_and_send(msg):
    # --- STEP 1: PKCS#7 padding ---
    padded_data = pad(msg.encode(), 16)

    # --- STEP 2: REAL CBC encryption (matches AESLib) ---
    cipher = AES.new(AES_KEY, AES.MODE_CBC, AES_IV_MASTER)
    encrypted_data = cipher.encrypt(padded_data)

    # Split into 16-byte blocks
    blocks = [encrypted_data[i:i+16] for i in range(0, len(encrypted_data), 16)]

    print(f"Sending {len(blocks)} blocks...")

    # --- STEP 3: Send blocks ---
    for i, block in enumerate(blocks):
        lora.beginPacket()
        lora.write(list(block), 16)
        lora.endPacket()
        lora.wait()

        print(
            f"Block {i+1}/{len(blocks)} Sent: "
            + " ".join(f"{b:02X}" for b in block)
        )

        time.sleep(0.1)

    print("Transmission complete.\n")

try:
    while True:
        user_input = input("Enter message: ").strip()
        if user_input:
            encrypt_and_send(user_input)
except KeyboardInterrupt:
    print("\nStopping...")
