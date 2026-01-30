# LoRa SX1278 Transmitter Using Raspberry Pi 4

This project demonstrates **LoRa transmission from a Raspberry Pi 4 (TX only)** using an **SX1278 module**, with reception handled by an **Arduino**.

---

## Hardware Used

- Raspberry Pi 4  
- SX1278 LoRa Module (433 MHz)  
- Jumper wires  
- Arduino (Receiver side – not covered here)

---

## SX1278 ↔ Raspberry Pi Pin Connections

| SX1278 Pin | Raspberry Pi GPIO | Physical Pin | Function |
|-----------|------------------|--------------|----------|
| VCC | 3V3 | Pin 1 | 3.3V Power |
| GND | GND | Pin 6 | Ground |
| SCK | GPIO11 | Pin 23 | SPI Clock |
| MISO | GPIO9 | Pin 21 | SPI MISO |
| MOSI | GPIO10 | Pin 19 | SPI MOSI |
| NSS / CS | GPIO8 (CE0) | Pin 24 | SPI Chip Select |
| RST | GPIO17 | Pin 11 | Reset |
| DIO0 | GPIO4 | Pin 7 | Interrupt (Packet Ready) |

---

## Raspberry Pi Configuration

Enable SPI interface:

```bash
sudo raspi-config
```

Navigate to:

```
Interface Options → SPI → Enable
```

Reboot:

```bash
sudo reboot
```

---

## Project Setup

```bash
sudo apt update
sudo apt install -y swig python3-dev build-essential
sudo apt install -y liblgpio-dev
sudo apt install -y python3-picamera2
```

```bash
cd Desktop
mkdir LoraTest
cd LoraTest
python -m venv lora_env
source lora_env/bin/activate
```

---

## Install Required Libraries

```bash
pip install spidev==3.8 rpi-lgpio==0.6 lgpio==0.2.2.0 LoRaRF==1.4.0 pycryptodome
```
---

## Transmitter Code (tx.py)
```bash
nano tx.py
```

```python
from LoRaRF import SX127x
import time

bus = 0
cs = 0
reset = 17
dio0 = 4

lora = SX127x()

if not lora.begin(bus, cs, reset, dio0):
    print("LoRa initialization failed!")
    exit()

lora.setFrequency(433000000)

print("Sending...")
lora.beginPacket()
lora.write(list("Hello".encode()), 5)
lora.endPacket()
lora.wait()
print("Sent!")
```
```bash
Ctrl + o -> Enter -> Ctrl + x
```
---

## Receiver Code (rx.py)
```bash
nano rx.py
```

```python
from LoRaRF import SX127x
import time

# Pin definitions (Matching your tx.py)
bus = 0
cs = 0
reset = 17
dio0 = 4

lora = SX127x()

# Initialize
if not lora.begin(bus, cs, reset, dio0):
    print("LoRa initialization failed!")
    exit()

lora.setFrequency(433000000)

print("Waiting for messages...")

try:
    while True:
        # Request receiver mode
        lora.request()
        lora.wait()
        
        # Check if a packet was received
        if lora.available():
            # Read payload
            payload = lora.read(lora.available())
            message = "".join([chr(x) for x in payload])
            
            # Get signal quality stats
            rssi = lora.packetRssi()
            snr = lora.packetSnr()
            
            print(f"Received: {message} | RSSI: {rssi} dBm | SNR: {snr} dB")
            
except KeyboardInterrupt:
    print("\nReceiver stopped.")
```
```bash
Ctrl + o -> Enter -> Ctrl + x
```
---

## Run the Transmitter

```bash
python tx.py
```
Expected output:

```
Sending...
Sent!
```

## Run the Receiver

```bash
python rx.py
```

---

## Notes

- Raspberry Pi acts only as **transmitter**
- Arduino is used as **receiver**
- Frequency must match on both sides
- SX1278 must be powered with **3.3V only**

---

## Troubleshooting

| Issue | Possible Cause |
|-----|---------------|
| SX1278 not detected | SPI disabled, wrong wiring |
| No data received | Frequency / SF / BW mismatch |
| Unstable behavior | Power supply or loose wires |



# pip install pycryptodome
