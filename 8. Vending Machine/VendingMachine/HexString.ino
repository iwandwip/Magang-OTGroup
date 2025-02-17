void writeHexString(String hexString) {
  // Memastikan string memiliki panjang genap
  if (hexString.length() % 2 != 0) {
    Serial.println("Error: Invalid hex string length");
    return;
  }

  Serial.print("| send: ");

  for (int i = 0; i < hexString.length(); i += 2) {
    // Ambil 2 karakter hex
    String byteStr = hexString.substring(i, i + 2);
    // Konversi string hex ke byte
    byte value = (byte)strtol(byteStr.c_str(), NULL, 16);
    // Kirim byte melalui AltSerial
    altSerial.write(value);

    if (value < 0x10) Serial.print("0");
    Serial.print(value, HEX);
  }
  // Serial.println();
}

void writeAllTestData() {
  for (int i = 0; i < testDataLen; i++) {
    writeHexString(testDataArr[i]);
    delay(250);  // Delay antar frame untuk memastikan data terkirim
  }
}