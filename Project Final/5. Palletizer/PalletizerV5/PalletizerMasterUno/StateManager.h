#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#define ENABLE_MODULE_NODEF_DIGITAL_OUTPUT

#include "Kinematrix.h"

class StateManager {
public:
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
    LED_OFF = 3
  };

  typedef void (*StateChangeCallback)(SystemState state, const String& stateName);

  StateManager();
  void begin();
  void update();

  void setStateChangeCallback(StateChangeCallback callback);

  SystemState getState() const;
  bool setState(SystemState newState);
  String getStateName() const;

  void setupLedIndicators(int greenPin, int yellowPin, int redPin);

private:
  static const int MAX_LED_INDICATOR_SIZE = 3;
  DigitalOut* ledIndicator;
  bool hasLedIndicators;

  SystemState systemState;
  StateChangeCallback stateChangeCallback;

  void setLedIndicator(LedIndicator index);
  String stateToString(SystemState state) const;
};

#endif