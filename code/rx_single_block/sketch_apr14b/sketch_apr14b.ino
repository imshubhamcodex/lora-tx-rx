#include <SPI.h>
#include <LoRa.h>
#include <AESLib.h>

#define LORA_SS    4  
#define LORA_RST   5  
#define LORA_DIO0  16 

AESLib aesLib;
const int BLOCK_SIZE = 16;
byte aes_key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x8C, 0x53, 0x4A, 0xF1, 0xE1, 0x1A};

byte aes_iv[BLOCK_SIZE] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};


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

    // Decrypt
    byte decrypted[BLOCK_SIZE];
    aesLib.decrypt(encrypted, BLOCK_SIZE, decrypted, aes_key, 128, aes_iv);

    // Validate padding
    int padLen = decrypted[BLOCK_SIZE - 1];
    bool valid = validatePadding(decrypted, padLen);

    if (valid) {
      int originalLength = BLOCK_SIZE - padLen;

      Serial.print("Decrypted Message: '");
      for(int i = 0; i < originalLength; i++) {
        Serial.print((char)decrypted[i]);
      }
      Serial.println("'");
    } else {
      Serial.println("First invalid padding caused by noise — ignore once");
    }
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