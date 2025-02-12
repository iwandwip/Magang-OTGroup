void decodeModbusData(byte* data) {
  byte address = data[0];                             // Alamat perangkat
  byte function = data[1];                            // Kode fungsi
  uint16_t crcReceived = (data[19] << 8) | data[18];  // CRC yang diterima (2 byte terakhir)

  // Verifikasi CRC untuk memastikan data valid
  uint16_t crcCalculated = calculateCRC16Modbus(data, 18);  // Hitung CRC untuk data (termasuk alamat dan fungsi, tanpa CRC)
  if (crcReceived != crcCalculated) {
    Serial.println("CRC Error: Data corrupted!");
    return;
  }

  // Menampilkan informasi dasar
  Serial.print("| addr: ");
  Serial.print(address, HEX);
  Serial.print("| FC: ");
  Serial.print(function, HEX);

  // Decode berdasarkan kode fungsi
  if (function == 0x03) {         // Read Holding Registers
    uint8_t byteCount = data[2];  // Jumlah byte yang diterima dalam response
    Serial.print("| Byte Count: ");
    Serial.print(byteCount);

    // Menafsirkan nilai yang diterima, bergantung pada jumlah byte
    Serial.print("| recv data: ");
    for (int i = 3; i < 3 + byteCount; i++) {
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }

    // Misalnya, jika kita menganggap data sebagai 16-bit nilai (2 byte per register)
    for (int i = 3; i < 3 + byteCount; i += 2) {
      uint16_t registerValue = (data[i] << 8) | data[i + 1];  // Gabungkan dua byte menjadi satu nilai register
      Serial.print("| Register Value: ");
      Serial.println(registerValue);
    }
  } else {
    Serial.println("Unsupported Function");
  }
}