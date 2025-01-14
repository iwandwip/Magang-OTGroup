#include "EEPROM.h"
#include "Header.h"
#include "TimerOne.h"
#include "Utils.h"
#include "avr/wdt.h"

enum class SystemState {
  WAITING,
  FILLING,
  TRANSFER
};

struct WlcSensor {
  bool isTimerStarted;
  uint32_t count;
  float voltage;
  int adc;
  int raw;
  int state;
};

typedef bool RadarSensor;

uint16_t delay1;
uint16_t delay2;
uint32_t fillingStartTime;
uint32_t transferStartTime;

uint32_t thresholdStartTime;
uint32_t ledBuiltinTime;

WlcSensor wlcSensor;
RadarSensor radarSensor;
SystemState currentState;
// EEPROMLib eeprom;

MovingAverageFilter adcFilter(40);

void setup() {
  Serial.begin(9600);
  wdt_enable(WDTO_8S);
  initializePins();
  // loadSettingsFromEEPROM();
  Timer1.initialize(1000000);
  Timer1.attachInterrupt([]() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  });
  currentState = SystemState::WAITING;
  // currentState = static_cast<SystemState>(eeprom.readInt(EEPROM_ADDR_STATE));
  // fillingStartTime = eeprom.readUint32(EEPROM_ADDR_FILLING_TIME);
  // transferStartTime = eeprom.readUint32(EEPROM_ADDR_TRANSFER_TIME);
}

void loop() {
  wdt_reset();
  if (Serial.available() > 0) {
    // handleTesting();
    // handleSerialCommand();
  }
  readAllSensor();
  processSystemState();
  debug();
}

void initializePins() {
  pinMode(PIN_RADAR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_FILLING_PUMP, OUTPUT);
  pinMode(PIN_DOSING_PUMP, OUTPUT);
  pinMode(PIN_TRANSFER_PUMP, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // Initial state - all pumps off
  updatePumpStates(false, false, false);
}

void readAllSensor() {
  readWlcSensor(&wlcSensor);
  radarSensor = isRadarStable();
}

void processSystemState() {
  Timer1.setPeriod(getLedDelay(wlcSensor.count, VERIFICATION_TIME, 25, 1000));

  switch (currentState) {
    case SystemState::WAITING:
      if (wlcSensor.state) {
        updatePumpStates(true, true, false);  // Start filling and dosing
        fillingStartTime = millis();
        currentState = SystemState::FILLING;
      }
      break;

    case SystemState::FILLING:
      if (radarSensor || millis() - fillingStartTime >= FILLING_TIMEOUT) {
        updatePumpStates(false, false, true);  // Stop filling, start transfer
        transferStartTime = millis();
        currentState = SystemState::TRANSFER;
      }
      break;

    case SystemState::TRANSFER:
      if (!radarSensor || millis() - transferStartTime >= TRANSFER_TIMEOUT) {
        updatePumpStates(false, false, false);  // Stop all pumps
        currentState = SystemState::WAITING;
        wlcSensor.voltage = 0;
        wlcSensor.raw = 0;
        wlcSensor.state = 0;
        wlcSensor.isTimerStarted = 0;
      }
      break;
  }
}

void readWlcSensor(WlcSensor* _wlcSensor) {
  // _wlcSensor->voltage = 0;
  // for (int i = 0; i < VOLTAGE_SAMPLES; i++) {
  //   float voltage = abs(analogRead(PIN_AC_VOLTAGE) - 512);
  //   // float adcVoltage = (float(analogRead(PIN_AC_VOLTAGE)) * 5.0f) / 1023.0f;
  //   _wlcSensor->adc = analogRead(PIN_AC_VOLTAGE);
  //   _wlcSensor->voltage += voltage * voltage;
  // }
  // _wlcSensor->voltage /= VOLTAGE_SAMPLES;

  adcFilter.addMeasurement(analogRead(PIN_AC_VOLTAGE));
  _wlcSensor->adc = adcFilter.getFilteredValue();

  // if (_wlcSensor->voltage <= VOLTAGE_THRESHOLD) {
  if (_wlcSensor->adc <= ADC_THRESHOLD) {
    _wlcSensor->raw = false;
    _wlcSensor->isTimerStarted = false;
    _wlcSensor->count = 0;
  } else {
    _wlcSensor->raw = true;
    if (!_wlcSensor->isTimerStarted) {
      thresholdStartTime = millis();
      _wlcSensor->isTimerStarted = true;
    } else if (millis() - thresholdStartTime >= VERIFICATION_TIME_MS) {
      _wlcSensor->state = true;
      return;
    }
    _wlcSensor->count = uint32_t(millis() - thresholdStartTime) / 1000;
  }
  _wlcSensor->state = false;
  return;
}

bool isRadarStable() {
  static unsigned long lastChangeTime = 0;
  static bool lastState = false;
  static uint8_t stableCount = 0;

  const unsigned long DEBOUNCE_DELAY = 500;
  const uint8_t STABLE_THRESHOLD = 5;

  bool currentReading = readRadarSensor();
  if (currentReading != lastState) {
    lastChangeTime = millis();
    lastState = currentReading;
    stableCount = 0;
  } else if (getElapsedTime(lastChangeTime) > DEBOUNCE_DELAY) {
    stableCount = min(stableCount + 1, STABLE_THRESHOLD);
  }
  return (stableCount >= STABLE_THRESHOLD) ? lastState : false;
}

bool readRadarSensor() {
  const uint8_t SAMPLE_COUNT = 10;
  const uint8_t THRESHOLD = 8;
  const uint8_t SAMPLE_DELAY_MS = 30;
  uint8_t activeCount = 0;
  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    if (digitalRead(PIN_RADAR_SENSOR) == LOW) {
      activeCount++;
    }
    delay(SAMPLE_DELAY_MS);
  }
  return (activeCount >= THRESHOLD);
}

unsigned long getElapsedTime(unsigned long startTime) {
  return (millis() >= startTime) ? (millis() - startTime) : (0xFFFFFFFF - startTime + millis());
}

uint32_t getLedDelay(int16_t countNumber, int16_t maxNum, uint32_t delayMin, uint32_t delayMax) {
  float factor = (float)countNumber / maxNum;
  return delayMax * pow((float)delayMin / delayMax, factor) * 1000;  // mS to uS
}

void updatePumpStates(bool filling, bool dosing, bool transfer) {
  digitalWrite(PIN_FILLING_PUMP, filling);
  digitalWrite(PIN_DOSING_PUMP, dosing);
  digitalWrite(PIN_TRANSFER_PUMP, transfer);
}