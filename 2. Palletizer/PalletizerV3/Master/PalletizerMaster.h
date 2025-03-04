#ifndef PALLETIZER_MASTER_H
#define PALLETIZER_MASTER_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

#include "Kinematrix.h"
#include "SoftwareSerial.h"

class PalletizerMaster {
public:
  enum Command {
    CMD_NONE,
    CMD_START,
    CMD_ZERO,
    CMD_PAUSE,
    CMD_RESUME,
    CMD_RESET,
    CMD_SETSPEED  // New command
  };

  PalletizerMaster(int rxPin, int txPin);
  void begin();
  void update();
  static void onBluetoothDataWrapper(const String& data);
  static void onSlaveDataWrapper(const String& data);

private:
  static PalletizerMaster* instance;

  SoftwareSerial slaveCommSerial;
  EnhancedSerial bluetoothSerial;
  EnhancedSerial slaveSerial;
  EnhancedSerial debugSerial;

  Command currentCommand = CMD_NONE;

  void onBluetoothData(const String& data);
  void onSlaveData(const String& data);
  void sendCommandToAllSlaves(Command cmd);
  void parseCoordinateData(const String& data);
};

#endif