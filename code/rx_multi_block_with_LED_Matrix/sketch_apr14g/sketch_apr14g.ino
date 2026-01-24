// ================= LoRa + AES + MAX7219 RX =================

// LoRa (SX1278)
// RST  -> D0
// DIO0 -> D1
// SCK  -> D5
// MISO -> D6
// MOSI -> D7
// NSS  -> D8

// MAX7219 (8x32)
// DIN -> D2
// CLK -> D4
// CS  -> D3

#include <SPI.h>
#include <LoRa.h>
#include <AESLib.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>

// ================= LoRa Pins =================
#define LORA_SS    15   // D8
#define LORA_RST   16   // D0
#define LORA_DIO0  2    // D1

// ================= MAX7219 =================
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4        // 8x32 matrix

#define DISP_DATA_PIN  D2
#define DISP_CLK_PIN   D4
#define DISP_CS_PIN    D3

char displayBuf[500];
int lastRssi = 0;

MD_Parola display = MD_Parola(
  HARDWARE_TYPE,
  DISP_DATA_PIN,
  DISP_CLK_PIN,
  DISP_CS_PIN,
  MAX_DEVICES
);

// ================= AES =================
AESLib aesLib;
const int BLOCK_SIZE = 16;

byte aes_key[] = {
  0x2B, 0x7E, 0x15, 0x16,
  0x28, 0xAE, 0xD2, 0xA6,
  0xAB, 0xF7, 0x8C, 0x53,
  0x4A, 0xF1, 0xE1, 0x1A
};

byte aes_iv[BLOCK_SIZE] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};

byte aes_iv_master[BLOCK_SIZE] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};

// ================= Message Buffer =================
#define MAX_BLOCKS 20

byte decryptedBlocks[MAX_BLOCKS][BLOCK_SIZE];
int blockIndex = 0;
unsigned long lastPacketTime = 0;
const unsigned long TIMEOUT_MS = 2000;

// ================= Setup =================
void setup() {
  Serial.begin(115200);
  delay(500);

  // ---- LoRa ----
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed");
    while (true);
  }
  // LoRa.setSpreadingFactor(12);
  // LoRa.setSignalBandwidth(62.5E3);
  // LoRa.setCodingRate4(8);

  Serial.println("LoRa RX Ready");

  // ---- MAX7219 ----
  display.begin();
  display.setIntensity(3);
  display.displayClear();
  display.displayText(
    "RX READY",
    PA_CENTER,
    80,
    2000,
    PA_SCROLL_LEFT,
    PA_SCROLL_LEFT
  );
}

// // Just to Test TX
// void loop() {
//   int packetSize = LoRa.parsePacket();
//   if (packetSize > 0) {
//     Serial.print("Got packet, size: ");
//     Serial.println(packetSize);
//     while (LoRa.available()) {
//       Serial.print((char)LoRa.read()); // print raw byte as char
//     }
//     Serial.println();
//   }
// }

// ================= Loop =================
void loop() {
  display.displayAnimate();

  int packetSize = LoRa.parsePacket();

  if (packetSize == BLOCK_SIZE) {
    lastRssi = LoRa.packetRssi();
    if (blockIndex == 0) {
      memcpy(aes_iv, aes_iv_master, BLOCK_SIZE);
    } 
    
    byte encrypted[BLOCK_SIZE];
    for (int i = 0; i < BLOCK_SIZE; i++) {
      encrypted[i] = LoRa.read();
    }

    Serial.print("Received HEX: ");
    printHex(encrypted, BLOCK_SIZE);

    byte decrypted[BLOCK_SIZE];
    aesLib.decrypt(encrypted, BLOCK_SIZE, decrypted, aes_key, 128, aes_iv);

    if (blockIndex < MAX_BLOCKS) {
      memcpy(decryptedBlocks[blockIndex], decrypted, BLOCK_SIZE);
      blockIndex++;
      lastPacketTime = millis();
    }
  }

  if (blockIndex > 0 && millis() - lastPacketTime > TIMEOUT_MS) {
    processMessage();
    blockIndex = 0;
  }
}

// ================= Message Processing =================
void processMessage() {
  byte* lastBlock = decryptedBlocks[blockIndex - 1];
  int padLen = lastBlock[BLOCK_SIZE - 1];

  if (!validatePadding(lastBlock, padLen)) {
    Serial.println("Invalid padding");
    return;
  }

  int totalLength = blockIndex * BLOCK_SIZE - padLen;
  String msg = "";

  int count = 0;
  for (int b = 0; b < blockIndex; b++) {
    for (int i = 0; i < BLOCK_SIZE && count < totalLength; i++) {
      msg += (char)decryptedBlocks[b][i];
      count++;
    }
  }

  Serial.print("Decrypted Message: ");
  Serial.println(msg);

  Serial.print("RSSI: ");
  Serial.print(lastRssi);
  Serial.println(" dBm");

  String fullMsg = msg + "  RSSI:" + String(lastRssi) + "dBm";

  display.displayClear();
  display.displayReset();
  fullMsg.toCharArray(displayBuf, sizeof(displayBuf));
  display.displayText(
    displayBuf,
    PA_LEFT,
    80,
    2000,
    PA_SCROLL_LEFT,
    PA_SCROLL_LEFT
  );
}

// ================= Helpers =================
bool validatePadding(byte* data, int padLen) {
  if (padLen < 1 || padLen > BLOCK_SIZE) return false;
  for (int i = 1; i <= padLen; i++) {
    if (data[BLOCK_SIZE - i] != padLen) return false;
  }
  return true;
}

void printHex(byte* data, int length) {
  for (int i = 0; i < length; i++) {
    if (data[i] < 0x10) Serial.print("0");
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
