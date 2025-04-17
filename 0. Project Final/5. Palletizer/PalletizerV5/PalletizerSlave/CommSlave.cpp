#include "CommSlave.h"

CommSlave* CommSlave::instance = nullptr;

CommSlave::CommSlave(char id, int rxPin, int txPin)
  : slaveId(id),
    masterCommSerial(rxPin, txPin),
    commandCallback(nullptr) {
  instance = this;
}

void CommSlave::begin() {
  Serial.begin(9600);
  masterCommSerial.begin(9600);
  masterSerial.begin(&masterCommSerial);
  debugSerial.begin(&Serial);
  masterSerial.setDataCallback(onMasterDataWrapper);

  DEBUG_PRINTLN("COMM " + String(slaveId) + ": Initialized");
}

void CommSlave::update() {
  masterSerial.checkCallback();
}

void CommSlave::setCommandCallback(CommandCallback callback) {
  commandCallback = callback;
}

void CommSlave::sendFeedback(const String& message) {
  String feedback = String(slaveId) + ";" + message;
  masterSerial.println(feedback);
  DEBUG_PRINTLN("SLAVE→MASTER: " + feedback);
}

void CommSlave::onMasterDataWrapper(const String& data) {
  if (instance) {
    instance->onMasterData(data);
  }
}

void CommSlave::onMasterData(const String& data) {
  DEBUG_PRINTLN("MASTER→SLAVE: " + data);
  processCommand(data);
}

void CommSlave::processCommand(const String& data) {
  int firstSeparator = data.indexOf(';');
  if (firstSeparator == -1) return;

  String slaveIdStr = data.substring(0, firstSeparator);
  slaveIdStr.toLowerCase();

  if (slaveIdStr != String(slaveId) && slaveIdStr != "all") return;

  int secondSeparator = data.indexOf(';', firstSeparator + 1);
  int cmdCode = (secondSeparator == -1)
                  ? data.substring(firstSeparator + 1).toInt()
                  : data.substring(firstSeparator + 1, secondSeparator).toInt();

  DEBUG_PRINTLN("COMM " + String(slaveId) + ": Processing command " + String(cmdCode));

  if (commandCallback) {
    String params = (secondSeparator == -1) ? "" : data.substring(secondSeparator + 1);
    commandCallback(cmdCode, params);
  }
}