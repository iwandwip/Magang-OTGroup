#include "EEPROM.h"

const uint8_t PIN_RADAR_SENSOR = 2;
const uint8_t PIN_AC_VOLTAGE = A0;
const uint8_t PIN_FILLING_PUMP = 9;   // In6 - filling tank pump
const uint8_t PIN_DOSING_PUMP = 11;   // In5 - dosing pump
const uint8_t PIN_TRANSFER_PUMP = 8;  // In4 - transfer pump

const uint8_t EEPROM_ADDR_DELAY1 = 0;
const uint8_t EEPROM_ADDR_DELAY2 = 1;

const unsigned long TRANSFER_TIMEOUT = 200000;  // 200 seconds
const uint16_t VOLTAGE_SAMPLES = 500;
const uint16_t RADAR_SAMPLES = 200;
const float VOLTAGE_THRESHOLD = 500.0;

enum class SystemState {
  WAITING,
  FILLING,
  TRANSFER
};

uint16_t delay1;
uint16_t delay2;
unsigned long transferStartTime;
SystemState currentState;

void setup() {
  Serial.begin(9600);
  initializePins();
  loadSettingsFromEEPROM();
  currentState = SystemState::WAITING;
}

void loop() {
  if (Serial.available() > 0) {
    handleSerialCommand();
  }
  processSystemState();
}

void initializePins() {
  pinMode(PIN_RADAR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_FILLING_PUMP, OUTPUT);
  pinMode(PIN_DOSING_PUMP, OUTPUT);
  pinMode(PIN_TRANSFER_PUMP, OUTPUT);

  // Initial state - all pumps off
  updatePumpStates(false, false, false);
}

void loadSettingsFromEEPROM() {
  delay1 = EEPROM.read(EEPROM_ADDR_DELAY1);
  delay2 = EEPROM.read(EEPROM_ADDR_DELAY2);
}

void handleSerialCommand() {
  String data = Serial.readStringUntil('\n');
  int commaIndex = data.indexOf(',');

  if (commaIndex > 0) {
    String firstValue = data.substring(0, commaIndex);
    String secondValue = data.substring(commaIndex + 1);

    delay1 = firstValue.toInt();
    delay2 = secondValue.toInt();

    EEPROM.write(EEPROM_ADDR_DELAY1, delay1);
    EEPROM.write(EEPROM_ADDR_DELAY2, delay2);
  } else if (data == "download") {
    Serial.print(EEPROM.read(EEPROM_ADDR_DELAY1));
    Serial.print(",");
    Serial.println(EEPROM.read(EEPROM_ADDR_DELAY2));
  } else {
    Serial.println("ERROR");
  }
}

void processSystemState() {
  switch (currentState) {
    case SystemState::WAITING:
      if (isACPowerPresent()) {
        updatePumpStates(true, true, false);  // Start filling and dosing, transfer
        currentState = SystemState::FILLING;
      }
      break;

    case SystemState::FILLING:
      if (isWaterTankFull()) {
        updatePumpStates(false, true, true);  // Stop filling, start transfer
        transferStartTime = millis();
        currentState = SystemState::TRANSFER;
      }
      break;

    case SystemState::TRANSFER:
      if (millis() - transferStartTime > TRANSFER_TIMEOUT) {
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
  return sumVoltage > VOLTAGE_THRESHOLD;
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