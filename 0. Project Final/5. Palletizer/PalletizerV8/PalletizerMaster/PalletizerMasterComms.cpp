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
  while (bluetoothSerial.available() > 0) {
    rxIndicatorLed.on();
    char c = bluetoothSerial.read();

    switch (btState) {
      case WAITING_FOR_START:
        if (c == '#') {
          btState = READING_PAYLOAD;
          btBuffer = "";
        }
        break;

      case READING_PAYLOAD:
        if (c == '$') {
          btState = MESSAGE_COMPLETE;
        } else {
          btBuffer += c;
        }
        break;

      case MESSAGE_COMPLETE:
        btState = WAITING_FOR_START;
        break;
    }

    if (btState == MESSAGE_COMPLETE) {
      if (bluetoothDataCallback) {
        bluetoothDataCallback(btBuffer);
      }
      btState = WAITING_FOR_START;
    }
    rxIndicatorLed.off();
  }
}

void PalletizerMasterComms::checkSlaveData() {
  while (slaveSerial.available() > 0) {
    rxIndicatorLed.on();
    char c = slaveSerial.read();

    switch (slaveState) {
      case WAITING_FOR_START:
        if (c == '#') {
          slaveState = READING_PAYLOAD;
          slaveBuffer = "";
        }
        break;

      case READING_PAYLOAD:
        if (c == '$') {
          slaveState = MESSAGE_COMPLETE;
        } else {
          slaveBuffer += c;
        }
        break;

      case MESSAGE_COMPLETE:
        slaveState = WAITING_FOR_START;
        break;
    }

    if (slaveState == MESSAGE_COMPLETE) {
      if (slaveDataCallback) {
        slaveDataCallback(slaveBuffer);
      }
      slaveState = WAITING_FOR_START;
    }
    rxIndicatorLed.off();
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