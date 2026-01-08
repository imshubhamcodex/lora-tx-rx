// Rx in NodeMCU
// RST -> D0
// DIO0 -> D1
// DIO1 -> D2
// SCK -> D5
// MISO -> D6
// MOSI -> D7
// NSS -> D8

#include <SPI.h>
#include <LoRa.h>
#include <AESLib.h>

#define LORA_SS    15 
#define LORA_RST   16  
#define LORA_DIO0  2 

AESLib aesLib;
const int BLOCK_SIZE = 16;
byte aes_key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                  0xAB, 0xF7, 0x8C, 0x53, 0x4A, 0xF1, 0xE1, 0x1A};

byte aes_iv[BLOCK_SIZE] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};

#define MAX_BLOCKS 10  // Up to 10 blocks = 160 characters

byte decryptedBlocks[MAX_BLOCKS][BLOCK_SIZE];
int blockIndex = 0;
unsigned long lastPacketTime = 0;
unsigned long TIMEOUT_MS = 500;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed.");
    while (true);
  }

  Serial.println("RX Ready");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize == BLOCK_SIZE) {
    byte encrypted[BLOCK_SIZE];
    int i = 0;
    while (LoRa.available() && i < BLOCK_SIZE) {
      encrypted[i++] = LoRa.read();
    }

    Serial.print("Received HEX: ");
    printHex(encrypted, BLOCK_SIZE);

    // Decrypt block
    byte decrypted[BLOCK_SIZE];
    aesLib.decrypt(encrypted, BLOCK_SIZE, decrypted, aes_key, 128, aes_iv);

    // Store decrypted block
    if (blockIndex < MAX_BLOCKS) {
      memcpy(decryptedBlocks[blockIndex], decrypted, BLOCK_SIZE);
      blockIndex++;
      lastPacketTime = millis();  // Reset timeout timer
    }
  }

  // If we received blocks and no new packets in TIMEOUT_MS → print complete message
  if (blockIndex > 0 && millis() - lastPacketTime > TIMEOUT_MS) {
    Serial.println("Reassembling full message...");

    // Get padding length from last block
    byte* lastBlock = decryptedBlocks[blockIndex - 1];
    int padLen = lastBlock[BLOCK_SIZE - 1];

    // Validate padding
    if (!validatePadding(lastBlock, padLen)) {
      Serial.println("Invalid padding. Likely due to noise — ignored.");
      blockIndex = 0;
      return;
    }

    int totalLength = blockIndex * BLOCK_SIZE - padLen;

    // Reconstruct message
    Serial.print("Decrypted Message: '");
    int count = 0;
    for (int b = 0; b < blockIndex; b++) {
      for (int i = 0; i < BLOCK_SIZE && count < totalLength; i++) {
        Serial.print((char)decryptedBlocks[b][i]);
        count++;
      }
    }
    Serial.println("'");
    Serial.println(" ");
    blockIndex = 0;  // Reset for next message
  }
}

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
