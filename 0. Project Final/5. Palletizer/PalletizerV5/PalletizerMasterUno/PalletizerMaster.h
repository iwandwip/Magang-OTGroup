#ifndef PALLETIZER_MASTER_H
#define PALLETIZER_MASTER_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED
#define ENABLE_MODULE_NODEF_DIGITAL_OUTPUT

#include "CommManager.h"
#include "CommandQueue.h"
#include "StateManager.h"
#include "CommandProcessor.h"

class PalletizerMaster {
public:
  PalletizerMaster(int rxPin, int txPin, int indicatorPin = -1);
  void begin();
  void update();

private:
  static PalletizerMaster* instance;

  CommManager commManager;
  CommandQueue commandQueue;
  StateManager stateManager;
  CommandProcessor commandProcessor;

  bool sequenceRunning;
  bool waitingForCompletion;
  bool requestNextCommand;

  int indicatorPin;
  bool indicatorEnabled;
  unsigned long lastCheckTime;

  void onBluetoothData(const String& data);
  void onSlaveData(const String& data);
  void onStateChanged(StateManager::SystemState state, const String& stateName);
  void onCommandOut(const String& slaveId, int command, const String& params);
  void onStateCommand(const String& command);

  void processNextCommand();
  void requestCommand();

  bool checkAllSlavesCompleted();
};

#endif