#ifndef COMM_MANAGER_H
#define COMM_MANAGER_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

#include "Kinematrix.h"
#include "SoftwareSerial.h"

#define DEBUG 0

#if DEBUG
#define DEBUG_PRINT(x) debugSerial.print(x)
#define DEBUG_PRINTLN(x) debugSerial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

class CommManager {
public:
  typedef void (*BluetoothDataCallback)(const String& data);
  typedef void (*SlaveDataCallback)(const String& data);

  CommManager(int rxPin, int txPin);
  void begin();
  void update();

  void setBluetoothDataCallback(BluetoothDataCallback callback);
  void setSlaveDataCallback(SlaveDataCallback callback);

  void sendToSlave(const String& command);
  void sendToBluetooth(const String& message);

private:
  static CommManager* instance;
  static void onBluetoothDataWrapper(const String& data);
  static void onSlaveDataWrapper(const String& data);

  SoftwareSerial slaveCommSerial;
  EnhancedSerial bluetoothSerial;
  EnhancedSerial slaveSerial;
  EnhancedSerial debugSerial;

  BluetoothDataCallback bluetoothCallback;
  SlaveDataCallback slaveCallback;
};

#endif