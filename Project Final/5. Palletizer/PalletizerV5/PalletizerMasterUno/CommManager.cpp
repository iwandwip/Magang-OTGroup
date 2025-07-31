#include "CommManager.h"

CommManager* CommManager::instance = nullptr;

CommManager::CommManager(int rxPin, int txPin)
  : slaveCommSerial(rxPin, txPin),
    bluetoothCallback(nullptr),
    slaveCallback(nullptr) {
  instance = this;
}

void CommManager::begin() {
  Serial.begin(9600);
  slaveCommSerial.begin(9600);

  bluetoothSerial.begin(&Serial);
  slaveSerial.begin(&slaveCommSerial);
  debugSerial.begin(&Serial);

  bluetoothSerial.setDataCallback(onBluetoothDataWrapper);
  slaveSerial.setDataCallback(onSlaveDataWrapper);

  DEBUG_PRINTLN("COMM: System initialized");
}

void CommManager::update() {
  bluetoothSerial.checkCallback();
  slaveSerial.checkCallback();
}

void CommManager::setBluetoothDataCallback(BluetoothDataCallback callback) {
  bluetoothCallback = callback;
}

void CommManager::setSlaveDataCallback(SlaveDataCallback callback) {
  slaveCallback = callback;
}

void CommManager::sendToSlave(const String& command) {
  slaveSerial.println(command);
  DEBUG_PRINTLN("MASTER→SLAVE: " + command);
}

void CommManager::sendToBluetooth(const String& message) {
  bluetoothSerial.println(message);
  DEBUG_PRINTLN("MASTER→ANDROID: " + message);
}

void CommManager::onBluetoothDataWrapper(const String& data) {
  if (instance && instance->bluetoothCallback) {
    DEBUG_PRINTLN("ANDROID→MASTER: " + data);
    instance->bluetoothCallback(data);
  }
}

void CommManager::onSlaveDataWrapper(const String& data) {
  if (instance && instance->slaveCallback) {
    DEBUG_PRINTLN("SLAVE→MASTER: " + data);
    instance->slaveCallback(data);
  }
}