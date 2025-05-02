#include "PalletizerMasterComms.h"

PalletizerMasterComms* PalletizerMasterComms::instance = nullptr;

PalletizerMasterComms::PalletizerMasterComms(int rxPin, int txPin)
  : rxPin(rxPin), txPin(txPin), rxIndicatorLed(2) {
  instance = this;
}

void PalletizerMasterComms::begin() {
  Serial.begin(9600);
  slaveCommSerial.begin(9600, SERIAL_8N1, rxPin, txPin);

  SerialBT.begin("ESP32_Palletizer");
  bluetoothSerial.begin(&SerialBT);
  slaveSerial.begin(&slaveCommSerial);
}

void PalletizerMasterComms::update() {
  checkBluetoothData();
  checkSlaveData();
}

void PalletizerMasterComms::setBluetoothDataCallback(DataCallback callback) {
  bluetoothDataCallback = callback;
}

void PalletizerMasterComms::setSlaveDataCallback(DataCallback callback) {
  slaveDataCallback = callback;
}

void PalletizerMasterComms::sendToSlave(const String& data) {
  slaveSerial.println(data);
}

void PalletizerMasterComms::sendToBluetooth(const String& data) {
  bluetoothSerial.println(data);
}

void PalletizerMasterComms::checkBluetoothData() {
  if (bluetoothSerial.available() > 0) {
    while (bluetoothSerial.available() > 0) {
      rxIndicatorLed.on();
      char c = bluetoothSerial.read();

      if (c == '\n' || c == '\r') {
        if (btPartialBuffer.length() > 0) {
          btPartialBuffer.trim();
          if (bluetoothDataCallback) {
            bluetoothDataCallback(btPartialBuffer);
          }
          btPartialBuffer = "";
        }
      } else {
        btPartialBuffer += c;
      }
      rxIndicatorLed.off();
    }
  }
}

void PalletizerMasterComms::checkSlaveData() {
  if (slaveSerial.available() > 0) {
    while (slaveSerial.available() > 0) {
      rxIndicatorLed.on();
      char c = slaveSerial.read();

      if (c == '\n' || c == '\r') {
        if (slavePartialBuffer.length() > 0) {
          slavePartialBuffer.trim();
          if (slaveDataCallback) {
            slaveDataCallback(slavePartialBuffer);
          }
          slavePartialBuffer = "";
        }
      } else {
        slavePartialBuffer += c;
      }
      rxIndicatorLed.off();
    }
  }
}

void PalletizerMasterComms::onBluetoothDataWrapper(const String& data) {
  if (instance && instance->bluetoothDataCallback) {
    instance->bluetoothDataCallback(data);
  }
}

void PalletizerMasterComms::onSlaveDataWrapper(const String& data) {
  if (instance && instance->slaveDataCallback) {
    instance->slaveDataCallback(data);
  }
}