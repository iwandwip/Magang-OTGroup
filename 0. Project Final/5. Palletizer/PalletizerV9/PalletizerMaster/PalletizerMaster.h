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

#define QUEUE_MODE_APPEND 0
#define QUEUE_MODE_OVERWRITE 1
#define QUEUE_OPERATION_MODE QUEUE_MODE_APPEND

#define SYNC_SET_PIN 25
#define SYNC_WAIT_PIN 27

#include "Kinematrix.h"
#include "FS.h"
#include "LittleFS.h"
#include "PalletizerScriptParser.h"

class PalletizerMaster {
public:
  enum Command {
    CMD_NONE = 0,
    CMD_RUN = 1,
    CMD_ZERO = 2,
    CMD_SETSPEED = 6,
    CMD_SET = 7,
    CMD_WAIT = 8
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

  enum WaitTimeoutStrategy {
    TIMEOUT_SKIP_CONTINUE = 0,
    TIMEOUT_PAUSE_SYSTEM = 1,
    TIMEOUT_ABORT_RESET = 2,
    TIMEOUT_RETRY_BACKOFF = 3
  };

  struct TimeoutConfig {
    unsigned long maxWaitTime = 30000;
    WaitTimeoutStrategy strategy = TIMEOUT_SKIP_CONTINUE;
    int maxTimeoutWarning = 5;
    int autoRetryCount = 0;
    bool saveToFile = true;
  };

  struct TimeoutStats {
    int totalTimeouts = 0;
    int successfulWaits = 0;
    unsigned long lastTimeoutTime = 0;
    unsigned long totalWaitTime = 0;
    int currentRetryCount = 0;
  };

  typedef void (*DataCallback)(const String& data);

  PalletizerMaster(int rxPin, int txPin, int indicatorPin = -1);
  void begin();
  void update();
  void sendToSlave(const String& data);
  void setSlaveDataCallback(DataCallback callback);
  static void processCommand(const String& data);
  SystemState getSystemState();
  void setTimeoutConfig(const TimeoutConfig& config);
  TimeoutConfig getTimeoutConfig();
  void setWaitTimeout(unsigned long timeoutMs);
  void setTimeoutStrategy(WaitTimeoutStrategy strategy);
  void setMaxTimeoutWarning(int maxWarning);
  TimeoutStats getTimeoutStats();
  void clearTimeoutStats();
  float getTimeoutSuccessRate();
  bool saveTimeoutConfig();
  bool loadTimeoutConfig();

private:
  static PalletizerMaster* instance;

  int rxPin, txPin;
  HardwareSerial& slaveCommSerial = Serial2;
  DigitalOut rxIndicatorLed;
  EnhancedSerial slaveSerial;
  String slavePartialBuffer = "";
  DataCallback slaveDataCallback = nullptr;

  static const int MAX_LED_INDICATOR_SIZE = 3;
  DigitalOut ledIndicator[MAX_LED_INDICATOR_SIZE];

  Command currentCommand = CMD_NONE;
  SystemState systemState = STATE_IDLE;
  bool sequenceRunning = false;
  bool waitingForCompletion = false;

  int indicatorPin;
  bool indicatorEnabled;
  unsigned long lastCheckTime = 0;
  unsigned long lastYieldTime = 0;

  bool requestNextCommand = false;
  bool scriptProcessing = false;
  bool queueClearRequested = false;

  const String queueFilePath = "/queue.txt";
  const String queueIndexPath = "/queue_index.txt";
  const String timeoutConfigPath = "/timeout_config.json";
  int queueSize = 0;
  int queueHead = 0;

  int syncSetPin;
  int syncWaitPin;
  bool waitingForSync;
  unsigned long waitStartTime;

  TimeoutConfig timeoutConfig;
  TimeoutStats timeoutStats;
  unsigned long waitStartTimeForStats;

  PalletizerScriptParser scriptParser;

  void checkSlaveData();
  void onCommandReceived(const String& data);
  void onSlaveData(const String& data);
  void processStandardCommand(const String& command);
  void processSpeedCommand(const String& data);
  void processCoordinateData(const String& data);
  void processSystemStateCommand(const String& command);
  void processSyncCommand(const String& command);
  void processSetCommand(const String& data);
  void processWaitCommand();
  void processScriptCommand(const String& script);
  void sendCommandToAllSlaves(Command cmd);
  void parseCoordinateData(const String& data);
  bool checkAllSlavesCompleted();
  bool checkSyncTimeout();
  void addToQueue(const String& command);
  String getFromQueue();
  bool isQueueEmpty();
  bool isQueueFull();
  void processNextCommand();
  void requestCommand();
  void clearQueue();
  bool initFileSystem();
  bool writeQueueIndex();
  bool readQueueIndex();
  bool appendToQueueFile(const String& command);
  String readQueueCommandAt(int index);
  int getQueueCount();
  void setSystemState(SystemState newState);
  void sendStateUpdate();
  void setOnLedIndicator(LedIndicator index);
  void loadCommandsFromFile();
  void handleWaitTimeout();
  void resetTimeoutCounter();
  void updateTimeoutStats(bool success);
  void processCommandsBatch(const String& commands);
  void safeYield();
  bool shouldClearQueue(const String& data);
  void ensureFileIsClosed(File& file);
};

#endif