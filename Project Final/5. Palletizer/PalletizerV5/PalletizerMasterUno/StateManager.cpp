#include "StateManager.h"

StateManager::StateManager()
  : hasLedIndicators(false),
    systemState(STATE_IDLE),
    stateChangeCallback(nullptr) {
  ledIndicator = new DigitalOut[MAX_LED_INDICATOR_SIZE];
}

void StateManager::begin() {
  setState(STATE_IDLE);
}

void StateManager::update() {
  if (hasLedIndicators) {
    for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
      ledIndicator[i].update();
    }
  }
}

void StateManager::setStateChangeCallback(StateChangeCallback callback) {
  stateChangeCallback = callback;
}

StateManager::SystemState StateManager::getState() const {
  return systemState;
}

bool StateManager::setState(SystemState newState) {
  if (systemState != newState) {
    systemState = newState;

    // Update LED indicators based on new state
    switch (systemState) {
      case STATE_IDLE:
        setLedIndicator(LED_RED);
        break;
      case STATE_RUNNING:
        setLedIndicator(LED_GREEN);
        break;
      case STATE_PAUSED:
        setLedIndicator(LED_YELLOW);
        break;
      case STATE_STOPPING:
        setLedIndicator(LED_RED);
        break;
      default:
        setLedIndicator(LED_OFF);
        break;
    }

    // Notify callback if set
    if (stateChangeCallback) {
      stateChangeCallback(systemState, getStateName());
    }

    return true;
  }
  return false;
}

String StateManager::getStateName() const {
  return stateToString(systemState);
}

void StateManager::setupLedIndicators(int greenPin, int yellowPin, int redPin) {
  if (!hasLedIndicators) {
    ledIndicator[LED_GREEN] = DigitalOut(greenPin, true);
    ledIndicator[LED_YELLOW] = DigitalOut(yellowPin, true);
    ledIndicator[LED_RED] = DigitalOut(redPin, true);
    hasLedIndicators = true;
  }
}

void StateManager::setLedIndicator(LedIndicator index) {
  if (!hasLedIndicators) return;

  for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
    ledIndicator[i].off();
  }

  if (index >= MAX_LED_INDICATOR_SIZE || index == LED_OFF) {
    return;
  }

  ledIndicator[index].on();
}

String StateManager::stateToString(SystemState state) const {
  switch (state) {
    case STATE_IDLE:
      return "IDLE";
    case STATE_RUNNING:
      return "RUNNING";
    case STATE_PAUSED:
      return "PAUSED";
    case STATE_STOPPING:
      return "STOPPING";
    default:
      return "UNKNOWN";
  }
}