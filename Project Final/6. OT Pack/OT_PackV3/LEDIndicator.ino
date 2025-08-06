// ========================================
// LEDIndicator.ino - LED Status Indicator System
// TimerOne-based LED blinking for system status indication
// ========================================

#include <TimerOne.h>

// LED State Enumeration
typedef enum {
  LED_OFF,        // LED disabled
  LED_IDLE,       // 1000ms - System idle/ready
  LED_EXTEND,     // 100ms - Extending motion  
  LED_RETRACT,    // 500ms - Retracting motion
  LED_ERROR,      // 50ms - Error condition
  LED_DEBUG       // 200ms - Debug mode active
} LedState;

// LED System Variables
volatile LedState currentLedState = LED_IDLE;
volatile bool ledToggleState = false;
volatile bool ledEnabled = true;
volatile unsigned long ledBlinkCount = 0;

// LED Configuration Variables (can be modified via serial)
unsigned long ledIdlePeriod = LED_IDLE_PERIOD;
unsigned long ledExtendPeriod = LED_EXTEND_PERIOD;
unsigned long ledRetractPeriod = LED_RETRACT_PERIOD;
unsigned long ledErrorPeriod = LED_ERROR_PERIOD;
unsigned long ledDebugPeriod = LED_DEBUG_PERIOD;

// Timer interrupt handler - called by TimerOne
void ledTimerISR() {
  if (!ledEnabled || currentLedState == LED_OFF) {
    digitalWrite(LED_BUILTIN, LOW);
    return;
  }
  
  // Toggle LED state
  ledToggleState = !ledToggleState;
  digitalWrite(LED_BUILTIN, ledToggleState);
  
  // Increment blink counter (for monitoring)
  if (ledToggleState) {
    ledBlinkCount++;
  }
}

// Initialize LED indicator system
void initializeLED() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  // Initialize TimerOne with idle period
  Timer1.initialize(ledIdlePeriod);
  Timer1.attachInterrupt(ledTimerISR);
  
  currentLedState = LED_IDLE;
  ledEnabled = true;
  
  Serial.println(F("LED Indicator system initialized"));
}

// Set LED blink state with immediate effect
void setLedState(LedState newState) {
  // Protect against interrupt conflicts
  noInterrupts();
  currentLedState = newState;
  interrupts();
  
  unsigned long newPeriod;
  
  switch(newState) {
    case LED_OFF:
      Timer1.detachInterrupt();
      digitalWrite(LED_BUILTIN, LOW);
      return;
      
    case LED_IDLE:
      newPeriod = ledIdlePeriod;
      break;
      
    case LED_EXTEND:
      newPeriod = ledExtendPeriod;
      break;
      
    case LED_RETRACT:
      newPeriod = ledRetractPeriod;
      break;
      
    case LED_ERROR:
      newPeriod = ledErrorPeriod;
      break;
      
    case LED_DEBUG:
      newPeriod = ledDebugPeriod;
      break;
      
    default:
      newPeriod = ledIdlePeriod;
      break;
  }
  
  // Update timer period
  Timer1.setPeriod(newPeriod);
  
  // Ensure interrupt is attached
  if (!Timer1.isRunning) {
    Timer1.attachInterrupt(ledTimerISR);
  }
}

// Enable/disable LED system
void setLedEnabled(bool enabled) {
  noInterrupts();
  ledEnabled = enabled;
  interrupts();
  
  if (!enabled) {
    digitalWrite(LED_BUILTIN, LOW);
  }
}

// Get current LED state as string
const char* getLedStateString() {
  switch(currentLedState) {
    case LED_OFF:     return "OFF";
    case LED_IDLE:    return "IDLE";
    case LED_EXTEND:  return "EXTEND";
    case LED_RETRACT: return "RETRACT";
    case LED_ERROR:   return "ERROR";
    case LED_DEBUG:   return "DEBUG";
    default:          return "UNKNOWN";
  }
}

// Update LED periods (for runtime configuration)
void updateLedPeriod(LedState state, unsigned long newPeriod) {
  switch(state) {
    case LED_IDLE:    ledIdlePeriod = newPeriod; break;
    case LED_EXTEND:  ledExtendPeriod = newPeriod; break;
    case LED_RETRACT: ledRetractPeriod = newPeriod; break;
    case LED_ERROR:   ledErrorPeriod = newPeriod; break;
    case LED_DEBUG:   ledDebugPeriod = newPeriod; break;
    default: break;
  }
  
  // Update current period if it's the active state
  if (currentLedState == state) {
    Timer1.setPeriod(newPeriod);
  }
}

// Get LED statistics
void showLedInfo() {
  Serial.println(F("=== LED Indicator Information ==="));
  Serial.print(F("Current State: "));
  Serial.println(getLedStateString());
  Serial.print(F("Enabled: "));
  Serial.println(ledEnabled ? F("YES") : F("NO"));
  Serial.print(F("Blink Count: "));
  Serial.println(ledBlinkCount);
  Serial.println(F("--- LED Periods (ms) ---"));
  Serial.print(F("IDLE: "));
  Serial.println(ledIdlePeriod / 1000);
  Serial.print(F("EXTEND: "));
  Serial.println(ledExtendPeriod / 1000);
  Serial.print(F("RETRACT: "));
  Serial.println(ledRetractPeriod / 1000);
  Serial.print(F("ERROR: "));
  Serial.println(ledErrorPeriod / 1000);
  Serial.print(F("DEBUG: "));
  Serial.println(ledDebugPeriod / 1000);
  Serial.println(F("==============================="));
}

// Test all LED states sequentially
void testLedStates() {
  Serial.println(F("Testing LED states..."));
  
  const LedState testStates[] = {LED_IDLE, LED_EXTEND, LED_RETRACT, LED_ERROR, LED_DEBUG};
  const char* stateNames[] = {"IDLE", "EXTEND", "RETRACT", "ERROR", "DEBUG"};
  const int numStates = 5;
  
  LedState originalState = currentLedState;
  
  for (int i = 0; i < numStates; i++) {
    Serial.print(F("Testing "));
    Serial.print(stateNames[i]);
    Serial.println(F(" state..."));
    
    setLedState(testStates[i]);
    delay(2000); // Show each state for 2 seconds
  }
  
  // Return to original state
  setLedState(originalState);
  Serial.println(F("LED test completed"));
}