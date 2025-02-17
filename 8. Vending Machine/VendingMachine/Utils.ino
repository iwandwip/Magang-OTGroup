void decodeModbusData(byte* data) {
  static int messageCount = 0;
  static bool isNewSequence = false;
  static byte lastFunction = 0;

  byte address = data[0];
  byte function = data[1];
  uint16_t crcReceived = (data[19] << 8) | data[18];

  // Reset counter saat function 5 (awal sequence baru)
  if (function == 0x05) {
    messageCount = 1;
    isNewSequence = true;
    lastFunction = function;
    Serial.println();
    return;
  } else {
    if (lastFunction == 0x05) {
      messageCount = 1;
    } else {
      messageCount++;
    }
    lastFunction = function;
  }

  // Verifikasi CRC
  uint16_t crcCalculated = calculateCRC16Modbus(data, 18);
  if (crcReceived != crcCalculated) {
    Serial.println("| CRC Error: Data corrupted!");
    return;
  }

  // Decode registers
  uint16_t reg1 = (data[3] << 8) | data[4];    // Bytes 3-4
  uint16_t reg2 = (data[5] << 8) | data[6];    // Bytes 5-6  (Counter/ID)
  uint16_t reg3 = (data[7] << 8) | data[8];    // Bytes 7-8  (Data 1)
  uint16_t reg4 = (data[9] << 8) | data[10];   // Bytes 9-10 (Data 2)
  uint16_t reg5 = (data[11] << 8) | data[12];  // Bytes 11-12
  uint16_t reg6 = (data[13] << 8) | data[14];  // Bytes 13-14
  uint16_t reg7 = (data[15] << 8) | data[16];  // Bytes 15-16
  uint16_t reg8 = (data[17] << 8) | data[18];  // Bytes 17-18 (CRC)

  // Print basic info
  Serial.print("| Device addr: ");
  Serial.print(address, HEX);
  Serial.print("| Function: ");
  Serial.print(function, HEX);
  Serial.print("| Counter: 0x");
  Serial.print(reg2, HEX);
  Serial.print("| Data1: 0x");
  Serial.print(reg3, HEX);
  Serial.print("| Data2: 0x");
  Serial.print(reg4, HEX);
  Serial.print("| CRC: 0x");
  Serial.print(crcReceived, HEX);

  // Deteksi kondisi motor
  if (messageCount >= 3) {  // Mulai deteksi setelah pesan inisial
    // Kondisi 3: Motor Tidak Berputar
    if (reg2 == 0 && reg3 == 0 && reg4 == 0) {
      Serial.print("| Status: Motor Tidak Berputar");
    }
    // Kondisi 2: Motor Berputar dengan Hambatan
    else if (isContainsCXSuffix(data, 20)) {  // Check for C8 suffix in raw data
      Serial.print("| Status: Motor Berputar dengan Hambatan");
    }
    // Kondisi 1: Motor Berputar Normal
    else if (reg2 > 0 && reg3 <= 0x3E) {
      Serial.print("| Status: Motor Berputar Normal");
    }
  }

  Serial.println();

  // Reset sequence detection
  // if (messageCount >= 9 || (reg2 == 0 && messageCount >= 4)) {
  if (messageCount >= 9) {
    messageCount = 0;
    isNewSequence = false;
  }
}

bool isContainsCXSuffix(byte* data, int length) {
  // for (int i = 0; i < length - 1; i++) {
  //   if (data[i] == 0xC6 || data[i] == 0xC7 || data[i] == 0xC8) {
  //     return true;
  //   }
  // }
  if (data[11] > 0x00) {
    return true;
  }
  return false;
}