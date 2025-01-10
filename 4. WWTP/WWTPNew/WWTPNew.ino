#include "EEPROM.h"
#include "Header.h"

enum class SystemState {
  WAITING,
  FILLING,
  TRANSFER
};

uint16_t delay1;
uint16_t delay2;
uint32_t fillingStartTime;
uint32_t transferStartTime;

uint32_t thresholdStartTime = 0;
bool isTimerStarted = false;

SystemState currentState;

void setup() {
  Serial.begin(9600);
  initializePins();
  // loadSettingsFromEEPROM();
  currentState = SystemState::WAITING;
}

void loop() {
  if (Serial.available() > 0) {
    // handleTesting();
    handleSerialCommand();
  }
  processSystemState();
  debug();
}

void initializePins() {
  pinMode(PIN_RADAR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_FILLING_PUMP, OUTPUT);
  pinMode(PIN_DOSING_PUMP, OUTPUT);
  pinMode(PIN_TRANSFER_PUMP, OUTPUT);

  // Initial state - all pumps off
  updatePumpStates(false, false, false);
}

void processSystemState() {
  switch (currentState) {
    case SystemState::WAITING:
      if (isACPowerPresent()) {
        updatePumpStates(true, true, false);  // Start filling and dosing
        fillingStartTime = millis();
        currentState = SystemState::FILLING;
      }
      break;

    case SystemState::FILLING:
      if (isWaterTankFull() || millis() - fillingStartTime >= FILLING_TIMEOUT) {
        updatePumpStates(false, false, true);  // Stop filling, start transfer
        transferStartTime = millis();
        currentState = SystemState::TRANSFER;
      }
      break;

    case SystemState::TRANSFER:
      if (!isWaterTankFull() || millis() - transferStartTime >= TRANSFER_TIMEOUT) {
        updatePumpStates(false, false, false);  // Stop all pumps
        currentState = SystemState::WAITING;
      }
      break;
  }
}

bool isACPowerPresent() {
  float sumVoltage = 0;
  for (int i = 0; i < VOLTAGE_SAMPLES; i++) {
    float voltage = abs(analogRead(PIN_AC_VOLTAGE) - 512);
    sumVoltage += voltage * voltage;
  }
  sumVoltage /= VOLTAGE_SAMPLES;

  if (sumVoltage <= VOLTAGE_THRESHOLD) {
    isTimerStarted = false;
  } else {
    if (!isTimerStarted) {
      thresholdStartTime = millis();
      isTimerStarted = true;
    } else if (millis() - thresholdStartTime >= VERIFICATION_TIME) {
      return true;
    }
  }
  return false;
}

bool isWaterTankFull() {
  const uint8_t SAMPLE_COUNT = 10;
  const uint8_t THRESHOLD = 8;
  const uint8_t SAMPLE_DELAY_MS = 30;
  uint8_t lowCount = 0;
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    if (digitalRead(PIN_RADAR_SENSOR) == LOW) {
      lowCount++;
    }
    delay(SAMPLE_DELAY_MS);
  }
  return (lowCount >= THRESHOLD);
}

void updatePumpStates(bool filling, bool dosing, bool transfer) {
  digitalWrite(PIN_FILLING_PUMP, filling);
  digitalWrite(PIN_DOSING_PUMP, dosing);
  digitalWrite(PIN_TRANSFER_PUMP, transfer);
}