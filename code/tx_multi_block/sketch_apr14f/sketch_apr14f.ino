// Tx in Arduino
// DIO0 -> D2
// SCK -> D13
// MISO -> D12
// MOSI -> D11
// NSS -> D10
// RST -> D9

#include <SPI.h>
#include <LoRa.h>
#include <AESLib.h>

#define LORA_SS    10
#define LORA_RST   9
#define LORA_DIO0  2

AESLib aesLib;
const int BLOCK_SIZE = 16;
byte aes_key[] = {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6,
                  0xAB, 0xF7, 0x8C, 0x53, 0x4A, 0xF1, 0xE1, 0x1A};

char aes_iv[BLOCK_SIZE] = {
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

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("TX Ready");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed.");
    while (true);
  }
  // LoRa.setSpreadingFactor(12);
  // LoRa.setSignalBandwidth(62.5E3);
  // LoRa.setCodingRate4(8);
  // LoRa.setTxPower(20);

}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      Serial.println("Message: " + input);
      encryptAndSend(input);
    }
  }
}

void encryptAndSend(String msg) {
  
  memcpy(aes_iv, aes_iv_master, BLOCK_SIZE);  // RESET IV

  int msgLen = msg.length();
  int padLen = BLOCK_SIZE - (msgLen % BLOCK_SIZE);
  if (padLen == 0) padLen = BLOCK_SIZE;

  int totalLen = msgLen + padLen;
  byte padded[totalLen];
  memset(padded, 0, totalLen);
  msg.getBytes(padded, totalLen + 1);

  for (int i = 0; i < padLen; i++) {
    padded[msgLen + i] = (byte)padLen;
  }

  int blocks = totalLen / BLOCK_SIZE;

  for (int i = 0; i < blocks; i++) {
    byte encrypted[BLOCK_SIZE];
    aesLib.encrypt(padded + i * BLOCK_SIZE, BLOCK_SIZE, encrypted, aes_key, 128, aes_iv);
    
    LoRa.beginPacket();
    LoRa.write(encrypted, BLOCK_SIZE);
    LoRa.endPacket();

    Serial.print("Block "); Serial.print(i+1); Serial.print("/"); Serial.print(blocks); Serial.print(" Sent: ");
    printHex(encrypted, BLOCK_SIZE);
    delay(100);  // Avoid collision
  }

  Serial.println("All blocks sent.\n");
}

void printHex(byte* data, int length) {
  for (int i = 0; i < length; i++) {
    if (data[i] < 0x10) Serial.print('0');
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}
