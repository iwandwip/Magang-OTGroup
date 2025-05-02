#ifndef PALLETIZER_MASTER_COMMS_H
#define PALLETIZER_MASTER_COMMS_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED
#define ENABLE_MODULE_NODEF_DIGITAL_OUTPUT

#include "Kinematrix.h"
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth tidak tersedia atau tidak diaktifkan
#endif

enum ParserState {
  WAITING_FOR_START,
  READING_PAYLOAD,
  MESSAGE_COMPLETE
};

class PalletizerMasterComms {
public:
  typedef void (*DataCallback)(const String& data);

  PalletizerMasterComms(int rxPin, int txPin);
  void begin();
  void update();
  void setBluetoothDataCallback(DataCallback callback);
  void setSlaveDataCallback(DataCallback callback);
  void sendToSlave(const String& data);
  void sendToBluetooth(const String& data);

private:
  int rxPin, txPin;
  HardwareSerial& slaveCommSerial = Serial2;
  DigitalOut rxIndicatorLed;

  BluetoothSerial SerialBT;
  EnhancedSerial bluetoothSerial;
  EnhancedSerial slaveSerial;

  String btBuffer = "";
  String slaveBuffer = "";

  ParserState btState = WAITING_FOR_START;
  ParserState slaveState = WAITING_FOR_START;

  void checkBluetoothData();
  void checkSlaveData();

  static PalletizerMasterComms* instance;
  static void onBluetoothDataWrapper(const String& data);
  static void onSlaveDataWrapper(const String& data);

  DataCallback bluetoothDataCallback = nullptr;
  DataCallback slaveDataCallback = nullptr;
};

#endif