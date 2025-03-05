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

  PalletizerMaster(int rxPin, int txPin, int indicatorPin = -1);  // Added indicator pin parameter with default -1
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

  int indicatorPin;                   // Indicator pin to monitor all slaves
  bool indicatorEnabled;              // Flag to check if indicator is enabled
  bool waitingForCompletion = false;  // Flag to track if we're waiting for slaves to complete
  bool sequenceRunning = false;       // Flag to track if a sequence is running
  unsigned long lastCheckTime = 0;    // For timing indicator checks

  void onBluetoothData(const String& data);
  void onSlaveData(const String& data);
  void sendCommandToAllSlaves(Command cmd);
  void parseCoordinateData(const String& data);
  bool checkAllSlavesCompleted();  // Method to check if all slaves are completed
};

#endif