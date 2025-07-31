#ifndef COMM_SLAVE_H
#define COMM_SLAVE_H

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

class CommSlave {
public:
  typedef void (*CommandCallback)(int command, const String& params);

  CommSlave(char slaveId, int rxPin, int txPin);
  void begin();
  void update();
  void setCommandCallback(CommandCallback callback);
  void sendFeedback(const String& message);

private:
  static CommSlave* instance;
  static void onMasterDataWrapper(const String& data);

  char slaveId;
  CommandCallback commandCallback;

  SoftwareSerial masterCommSerial;
  EnhancedSerial masterSerial;
  EnhancedSerial debugSerial;

  void onMasterData(const String& data);
  void processCommand(const String& data);
};

#endif