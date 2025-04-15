#include <SPI.h>
#include <LoRa.h>
#include <AESLib.h>

#define LORA_SS    10
#define LORA_RST   9
#define LORA_DIO0  2

AESLib aesLib;
const int BLOCK_SIZE = 16;
byte aes_key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x8C, 0x53, 0x4A, 0xF1, 0xE1, 0x1A};

char aes_iv[BLOCK_SIZE] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};

void setup() {
  Serial.begin(9600);
  delay(1000); // Short delay instead of blocking
  Serial.println("TX Ready");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed.");
    while (true);
  }
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      processMessage(input);
    }
  }
}

void processMessage(String message) {
  byte padded[BLOCK_SIZE] = {0};
  int msgLen = message.length();
  int padLen = BLOCK_SIZE - (msgLen % BLOCK_SIZE);
  padLen = (padLen == 0) ? BLOCK_SIZE : padLen;
  
  // Copy message
  message.getBytes(padded, msgLen + 1);
  Serial.println(message);

  // Add PKCS7 padding
  for (int i = 0; i < padLen; i++) {
    padded[msgLen + i] = (byte)padLen;
  }

  // Encrypt
  byte encrypted[BLOCK_SIZE];
  aesLib.encrypt(padded, BLOCK_SIZE, encrypted, aes_key, 128, aes_iv);

  // Transmit encrypted data
  LoRa.beginPacket();
  LoRa.write(encrypted, BLOCK_SIZE);
  LoRa.endPacket();

  // Print encrypted HEX
  Serial.print("Encrypted HEX: ");
  printHex(encrypted, BLOCK_SIZE);
}

void printHex(byte* data, int length) {
  for (int i = 0; i < length; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}