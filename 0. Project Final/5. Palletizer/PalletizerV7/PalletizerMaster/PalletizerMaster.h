#ifndef PALLETIZER_MASTER_H
#define PALLETIZER_MASTER_H

#define ENABLE_MODULE_NODEF_SERIAL_ENHANCED
#define ENABLE_MODULE_NODEF_DIGITAL_OUTPUT

#define DEBUG 1

#if DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

#include "Kinematrix.h"
#include "PalletizerMasterComms.h"

class PalletizerMaster {
public:
  enum Command {
    CMD_NONE = 0,
    CMD_RUN = 1,
    CMD_ZERO = 2,
    CMD_SETSPEED = 6
  };

  enum SystemState {
    STATE_IDLE = 0,
    STATE_RUNNING = 1,
    STATE_PAUSED = 2,
    STATE_STOPPING = 3
  };

  enum LedIndicator {
    LED_GREEN = 0,
    LED_YELLOW = 1,
    LED_RED = 2,
    LED_OFF = 3,
  };

  PalletizerMaster(int rxPin, int txPin, int indicatorPin = -1);
  void begin();
  void update();
  static void onBluetoothDataWrapper(const String& data);
  static void onSlaveDataWrapper(const String& data);

private:
  static PalletizerMaster* instance;

  PalletizerMasterComms comms;

  static const int MAX_LED_INDICATOR_SIZE = 3;
  DigitalOut ledIndicator[MAX_LED_INDICATOR_SIZE];

  Command currentCommand = CMD_NONE;
  SystemState systemState = STATE_IDLE;
  bool sequenceRunning = false;
  bool waitingForCompletion = false;

  int indicatorPin;
  bool indicatorEnabled;
  unsigned long lastCheckTime = 0;

  static const int MAX_QUEUE_SIZE = 5;
  String commandQueue[MAX_QUEUE_SIZE];
  int queueHead = 0;
  int queueTail = 0;
  int queueSize = 0;
  bool requestNextCommand = false;

  void onBluetoothData(const String& data);
  void onSlaveData(const String& data);
  void processStandardCommand(const String& command);
  void processSpeedCommand(const String& data);
  void processCoordinateData(const String& data);
  void processSystemStateCommand(const String& command);
  void sendCommandToAllSlaves(Command cmd);
  void parseCoordinateData(const String& data);
  bool checkAllSlavesCompleted();
  void addToQueue(const String& command);
  String getFromQueue();
  bool isQueueEmpty();
  bool isQueueFull();
  void processNextCommand();
  void requestCommand();
  void clearQueue();
  void setSystemState(SystemState newState);
  void sendStateUpdate(bool send = false);
  void setOnLedIndicator(LedIndicator index);
};

#endif