#include "BluetoothSerial.h"

// Cek apakah Bluetooth tersedia
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth tidak tersedia atau tidak diaktifkan
#endif

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32_BT");  // Nama perangkat Bluetooth
  Serial.println("Bluetooth dimulai, siap untuk pairing!");
}

void loop() {
  // Kirim data dari Serial Monitor ke Bluetooth
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }

  // Kirim data dari Bluetooth ke Serial Monitor
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }

  delay(20);
}