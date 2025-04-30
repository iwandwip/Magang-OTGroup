#include "PalletizerMasterComms.h"

PalletizerMasterComms* PalletizerMasterComms::instance = nullptr;

PalletizerMasterComms::PalletizerMasterComms(int rxPin, int txPin)
  : rxPin(rxPin), txPin(txPin) {
  instance = this;
}

void PalletizerMasterComms::begin() {
  Serial.begin(9600);
  slaveCommSerial.begin(9600, SERIAL_8N1, rxPin, txPin);

  SerialBT.begin("ESP32_Palletizer");
  bluetoothSerial.begin(&SerialBT);
  slaveSerial.begin(&slaveCommSerial);

  bluetoothSerial.setDataCallback(onBluetoothDataWrapper);
  slaveSerial.setDataCallback(onSlaveDataWrapper);
}

void PalletizerMasterComms::update() {
  bluetoothSerial.checkCallback();
  slaveSerial.checkCallback();
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