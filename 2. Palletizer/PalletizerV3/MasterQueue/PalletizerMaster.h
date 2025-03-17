#ifndef PALLETIZER_MASTER_H
#define PALLETIZER_MASTER_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED

#include "Kinematrix.h"
#include "SoftwareSerial.h"

class PalletizerMaster {
public:
  enum Command {
    CMD_NONE = 0,
    CMD_RUN = 1,  // Renamed from CMD_START
    CMD_ZERO = 2,
    CMD_SETSPEED = 6
  };

  PalletizerMaster(int rxPin, int txPin, int indicatorPin = -1);
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
  bool sequenceRunning = false;
  bool waitingForCompletion = false;

  int indicatorPin;
  bool indicatorEnabled;
  unsigned long lastCheckTime = 0;

  void onBluetoothData(const String& data);
  void onSlaveData(const String& data);

  void processStandardCommand(const String& command);
  void processSpeedCommand(const String& data);
  void processCoordinateData(const String& data);

  void sendCommandToAllSlaves(Command cmd);
  void parseCoordinateData(const String& data);
  bool checkAllSlavesCompleted();
};

#endif