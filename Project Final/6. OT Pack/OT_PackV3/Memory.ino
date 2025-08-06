// ========================================
// Memory.ino - Memory Management Functions
// EEPROM storage and SRAM monitoring functions
// ========================================

// Function to get free SRAM (for memory monitoring)
int getFreeSRAM() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Function to calculate used SRAM percentage
float getSRAMUsage() {
  const int TOTAL_SRAM = 2048;  // Arduino Uno has 2KB SRAM
  int freeSRAM = getFreeSRAM();
  int usedSRAM = TOTAL_SRAM - freeSRAM;
  return ((float)usedSRAM / TOTAL_SRAM) * 100.0;
}

// EEPROM Functions
void saveToEEPROM() {
  // Save signature first
  EEPROM.put(EEPROM_SIGNATURE_ADDR, EEPROM_SIGNATURE);

  // Save motion parameters
  EEPROM.put(EXTEND_SPEED_ADDR, extendMaxSpeed);
  EEPROM.put(EXTEND_ACCEL_ADDR, extendAcceleration);
  EEPROM.put(EXTEND_DELAY_ADDR, extendDelayBefore);
  EEPROM.put(RETRACT_SPEED_ADDR, retractMaxSpeed);
  EEPROM.put(RETRACT_ACCEL_ADDR, retractAcceleration);
  EEPROM.put(RETRACT_DELAY_BEFORE_ADDR, retractDelayBefore);
  EEPROM.put(RETRACT_DELAY_AFTER_ADDR, retractDelayAfter);
  EEPROM.put(RETRACT_ADJUSTMENT_ADDR, retractStepAdjustment);

  // Save LED timing parameters
  EEPROM.put(LED_IDLE_PERIOD_ADDR, ledIdlePeriod);
  EEPROM.put(LED_EXTEND_PERIOD_ADDR, ledExtendPeriod);
  EEPROM.put(LED_RETRACT_PERIOD_ADDR, ledRetractPeriod);
  EEPROM.put(LED_ERROR_PERIOD_ADDR, ledErrorPeriod);
  EEPROM.put(LED_DEBUG_PERIOD_ADDR, ledDebugPeriod);

  Serial.println(F("Configuration saved to EEPROM"));
}

void loadFromEEPROM() {
  uint32_t signature;
  EEPROM.get(EEPROM_SIGNATURE_ADDR, signature);

  if (signature == EEPROM_SIGNATURE) {
    // Load motion parameters
    EEPROM.get(EXTEND_SPEED_ADDR, extendMaxSpeed);
    EEPROM.get(EXTEND_ACCEL_ADDR, extendAcceleration);
    EEPROM.get(EXTEND_DELAY_ADDR, extendDelayBefore);
    EEPROM.get(RETRACT_SPEED_ADDR, retractMaxSpeed);
    EEPROM.get(RETRACT_ACCEL_ADDR, retractAcceleration);
    EEPROM.get(RETRACT_DELAY_BEFORE_ADDR, retractDelayBefore);
    EEPROM.get(RETRACT_DELAY_AFTER_ADDR, retractDelayAfter);
    EEPROM.get(RETRACT_ADJUSTMENT_ADDR, retractStepAdjustment);

    // Load LED timing parameters
    EEPROM.get(LED_IDLE_PERIOD_ADDR, ledIdlePeriod);
    EEPROM.get(LED_EXTEND_PERIOD_ADDR, ledExtendPeriod);
    EEPROM.get(LED_RETRACT_PERIOD_ADDR, ledRetractPeriod);
    EEPROM.get(LED_ERROR_PERIOD_ADDR, ledErrorPeriod);
    EEPROM.get(LED_DEBUG_PERIOD_ADDR, ledDebugPeriod);

    Serial.println(F("Configuration loaded from EEPROM"));
  } else {
    Serial.println(F("EEPROM not initialized, using default values"));
    saveToEEPROM();  // Save defaults to EEPROM
  }
}

void resetEEPROM() {
  // Reset motion parameters to default values from Constants.h
  extendMaxSpeed = DEFAULT_EXTEND_MAX_SPEED;
  extendAcceleration = DEFAULT_EXTEND_ACCELERATION;
  extendDelayBefore = DEFAULT_EXTEND_DELAY_BEFORE;
  retractMaxSpeed = DEFAULT_RETRACT_MAX_SPEED;
  retractAcceleration = DEFAULT_RETRACT_ACCELERATION;
  retractDelayBefore = DEFAULT_RETRACT_DELAY_BEFORE;
  retractDelayAfter = DEFAULT_RETRACT_DELAY_AFTER;
  retractStepAdjustment = DEFAULT_RETRACT_STEP_ADJUSTMENT;

  // Reset LED timing parameters to defaults
  ledIdlePeriod = LED_IDLE_PERIOD;
  ledExtendPeriod = LED_EXTEND_PERIOD;
  ledRetractPeriod = LED_RETRACT_PERIOD;
  ledErrorPeriod = LED_ERROR_PERIOD;
  ledDebugPeriod = LED_DEBUG_PERIOD;

  // Save defaults to EEPROM
  saveToEEPROM();
  Serial.println(F("Configuration reset to defaults"));
}