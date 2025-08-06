// ========================================
// SerialCommand.ino - Serial Command Handling
// All serial communication and command processing
// ========================================

void serialCommander() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.startsWith("SET ")) {
      processSetCommand(command);
    } else if (command == "SHOW") {
      showConfiguration();
    } else if (command == "SAVE") {
      saveToEEPROM();
    } else if (command == "LOAD") {
      loadFromEEPROM();
    } else if (command == "RESET") {
      resetEEPROM();
    } else if (command == "DEBUG") {
      debugMode = !debugMode;
      Serial.print(F("Debug mode "));
      Serial.println(debugMode ? F("ENABLED") : F("DISABLED"));
    } else if (command == "MODE") {
      operationMode = (operationMode == MODE_NORMAL) ? MODE_TESTING : MODE_NORMAL;
      Serial.print(F("Operation mode: "));
      Serial.println(operationMode == MODE_NORMAL ? F("NORMAL") : F("TESTING"));
      if (operationMode == MODE_TESTING) {
        Serial.println(F("Testing mode enabled - use SENSOR_HIGH/SENSOR_LOW commands"));
      }
    } else if (command == "SENSOR_HIGH" && operationMode == MODE_TESTING) {
      sensorValue = HIGH;
      Serial.println(F("Simulated sensor set to HIGH"));
    } else if (command == "SENSOR_LOW" && operationMode == MODE_TESTING) {
      sensorValue = LOW;
      Serial.println(F("Simulated sensor set to LOW"));
    } else if (command == "DEBOUNCE_INFO") {
      showDebounceInfo();
    } else if (command == "LED_INFO") {
      showLedInfo();
    } else if (command == "LED_ON") {
      setLedEnabled(true);
      Serial.println(F("LED indicator enabled"));
    } else if (command == "LED_OFF") {
      setLedEnabled(false);
      Serial.println(F("LED indicator disabled"));
    } else if (command == "LED_TEST") {
      testLedStates();
    } else if (command == "HELP") {
      showHelp();
    } else {
      Serial.println(F("Unknown command. Type HELP for available commands."));
    }
  }
}

void processSetCommand(String command) {
  String parameter = command.substring(4);
  int equalIndex = parameter.indexOf('=');

  if (equalIndex > 0) {
    String paramName = parameter.substring(0, equalIndex);
    String paramValue = parameter.substring(equalIndex + 1);
    float value = paramValue.toFloat();

    paramName.toUpperCase();

    // Extend Motion Parameters
    if (paramName == "EXTEND_SPEED") {
      extendMaxSpeed = value;
      Serial.print(F("Extend speed set to: "));
      Serial.println(extendMaxSpeed);
      saveToEEPROM();
    } else if (paramName == "EXTEND_ACCEL") {
      extendAcceleration = value;
      Serial.print(F("Extend acceleration set to: "));
      Serial.println(extendAcceleration);
      saveToEEPROM();
    } else if (paramName == "EXTEND_DELAY") {
      extendDelayBefore = (int)value;
      Serial.print(F("Extend delay set to: "));
      Serial.print(extendDelayBefore);
      Serial.println(F("ms"));
      saveToEEPROM();
    }
    // Retract Motion Parameters
    else if (paramName == "RETRACT_SPEED") {
      retractMaxSpeed = value;
      Serial.print(F("Retract speed set to: "));
      Serial.println(retractMaxSpeed);
      saveToEEPROM();
    } else if (paramName == "RETRACT_ACCEL") {
      retractAcceleration = value;
      Serial.print(F("Retract acceleration set to: "));
      Serial.println(retractAcceleration);
      saveToEEPROM();
    } else if (paramName == "RETRACT_DELAY_BEFORE") {
      retractDelayBefore = (int)value;
      Serial.print(F("Retract delay before set to: "));
      Serial.print(retractDelayBefore);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else if (paramName == "RETRACT_DELAY_AFTER") {
      retractDelayAfter = (int)value;
      Serial.print(F("Retract delay after set to: "));
      Serial.print(retractDelayAfter);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else if (paramName == "RETRACT_ADJUSTMENT") {
      retractStepAdjustment = (int)value;
      Serial.print(F("Retract step adjustment set to: "));
      Serial.println(retractStepAdjustment);
      saveToEEPROM();
    }
    // Motion Smoothness Parameters
    else if (paramName == "JERK_STEPS") {
      jerkReductionSteps = (int)value;
      Serial.print(F("Jerk reduction steps set to: "));
      Serial.println(jerkReductionSteps);
    } else if (paramName == "ENABLE_RAMP") {
      enableRampDelay = (int)value;
      Serial.print(F("Enable ramp delay set to: "));
      Serial.print(enableRampDelay);
      Serial.println(F("ms"));
    } else if (paramName == "DISABLE_RAMP") {
      disableRampDelay = (int)value;
      Serial.print(F("Disable ramp delay set to: "));
      Serial.print(disableRampDelay);
      Serial.println(F("ms"));
    } else if (paramName == "SPEED_RAMP") {
      speedRampFactor = value;
      Serial.print(F("Speed ramp factor set to: "));
      Serial.println(speedRampFactor);
    } else if (paramName == "SETTLE_DELAY") {
      motionSettleDelay = (int)value;
      Serial.print(F("Motion settle delay set to: "));
      Serial.print(motionSettleDelay);
      Serial.println(F("ms"));
    }
    // LED Configuration Parameters (with EEPROM save)
    else if (paramName == "LED_IDLE") {
      updateLedPeriod(LED_IDLE, (unsigned long)(value * 1000));
      Serial.print(F("LED idle period set to: "));
      Serial.print(value);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else if (paramName == "LED_EXTEND") {
      updateLedPeriod(LED_EXTEND, (unsigned long)(value * 1000));
      Serial.print(F("LED extend period set to: "));
      Serial.print(value);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else if (paramName == "LED_RETRACT") {
      updateLedPeriod(LED_RETRACT, (unsigned long)(value * 1000));
      Serial.print(F("LED retract period set to: "));
      Serial.print(value);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else if (paramName == "LED_ERROR") {
      updateLedPeriod(LED_ERROR, (unsigned long)(value * 1000));
      Serial.print(F("LED error period set to: "));
      Serial.print(value);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else if (paramName == "LED_DEBUG") {
      updateLedPeriod(LED_DEBUG, (unsigned long)(value * 1000));
      Serial.print(F("LED debug period set to: "));
      Serial.print(value);
      Serial.println(F("ms"));
      saveToEEPROM();
    } else {
      Serial.print(F("Unknown parameter: "));
      Serial.println(paramName);
    }
  } else {
    Serial.println(F("Invalid format. Use: SET PARAMETER=VALUE"));
  }
}

void showConfiguration() {
  Serial.println(F("=== Current Configuration ==="));
  Serial.print(F("EXTEND_SPEED="));
  Serial.println(extendMaxSpeed);
  Serial.print(F("EXTEND_ACCEL="));
  Serial.println(extendAcceleration);
  Serial.print(F("EXTEND_DELAY="));
  Serial.println(extendDelayBefore);
  Serial.print(F("RETRACT_SPEED="));
  Serial.println(retractMaxSpeed);
  Serial.print(F("RETRACT_ACCEL="));
  Serial.println(retractAcceleration);
  Serial.print(F("RETRACT_DELAY_BEFORE="));
  Serial.println(retractDelayBefore);
  Serial.print(F("RETRACT_DELAY_AFTER="));
  Serial.println(retractDelayAfter);
  Serial.print(F("RETRACT_ADJUSTMENT="));
  Serial.println(retractStepAdjustment);
  Serial.println(F("--- Motion Smoothness ---"));
  Serial.print(F("JERK_STEPS="));
  Serial.println(jerkReductionSteps);
  Serial.print(F("ENABLE_RAMP="));
  Serial.println(enableRampDelay);
  Serial.print(F("DISABLE_RAMP="));
  Serial.println(disableRampDelay);
  Serial.print(F("SPEED_RAMP="));
  Serial.println(speedRampFactor);
  Serial.print(F("SETTLE_DELAY="));
  Serial.println(motionSettleDelay);
  Serial.println(F("--- System Information ---"));
  Serial.print(F("Operation Mode: "));
  Serial.println(operationMode == MODE_NORMAL ? F("NORMAL") : F("TESTING"));
  Serial.print(F("Debug Mode: "));
  Serial.println(debugMode ? F("ENABLED") : F("DISABLED"));
  Serial.print(F("Motor State: "));
  Serial.println(isExtended ? F("Extended") : F("Retracted"));
  Serial.print(F("Free SRAM: "));
  Serial.print(getFreeSRAM());
  Serial.print(F(" / 2048 bytes ("));
  Serial.print(getSRAMUsage(), 1);
  Serial.println(F("% used)"));
  Serial.print(F("Steps per Rev: "));
  Serial.println(stepsPerRevolution);
  Serial.println(F("--- LED Indicator Status ---"));
  Serial.print(F("LED State: "));
  Serial.println(getLedStateString());
  Serial.print(F("LED Enabled: "));
  Serial.println(ledEnabled ? F("YES") : F("NO"));
  Serial.println(F("--- LED Timing Parameters ---"));
  Serial.print(F("LED_IDLE="));
  Serial.println(ledIdlePeriod / 1000);
  Serial.print(F("LED_EXTEND="));
  Serial.println(ledExtendPeriod / 1000);
  Serial.print(F("LED_RETRACT="));
  Serial.println(ledRetractPeriod / 1000);
  Serial.print(F("LED_ERROR="));
  Serial.println(ledErrorPeriod / 1000);
  Serial.print(F("LED_DEBUG="));
  Serial.println(ledDebugPeriod / 1000);
  Serial.println(F("============================="));
}

void showDebounceInfo() {
  Serial.println(F("=== Debounce Information ==="));
  Serial.print(F("Raw sensor reading: "));
  Serial.println(digitalRead(SENSOR_PIN));
  Serial.print(F("Debounced reading: "));
  Serial.println(getDebouncedSensorReading());
  Serial.print(F("Debounce delay: "));
  Serial.print(DEBOUNCE_DELAY);
  Serial.println(F("ms"));
  Serial.print(F("Sample count: "));
  Serial.println(DEBOUNCE_SAMPLES);
  Serial.print(F("Sample interval: "));
  Serial.print(DEBOUNCE_SAMPLE_INTERVAL);
  Serial.println(F("ms"));
  Serial.println(F("==========================="));
}

void showHelp() {
  Serial.println(F("=== SerialCommander Help ==="));
  Serial.println(F("SET EXTEND_SPEED=value        - Set extend motion speed"));
  Serial.println(F("SET EXTEND_ACCEL=value        - Set extend acceleration"));
  Serial.println(F("SET EXTEND_DELAY=value        - Set extend delay (ms)"));
  Serial.println(F("SET RETRACT_SPEED=value       - Set retract motion speed"));
  Serial.println(F("SET RETRACT_ACCEL=value       - Set retract acceleration"));
  Serial.println(F("SET RETRACT_DELAY_BEFORE=val  - Set retract delay before (ms)"));
  Serial.println(F("SET RETRACT_DELAY_AFTER=val   - Set retract delay after (ms)"));
  Serial.println(F("SET RETRACT_ADJUSTMENT=val    - Set retract step adjustment"));
  Serial.println(F("--- Motion Smoothness Commands ---"));
  Serial.println(F("SET JERK_STEPS=val            - Set S-curve jerk reduction steps"));
  Serial.println(F("SET ENABLE_RAMP=val           - Set enable ramp delay (ms)"));
  Serial.println(F("SET DISABLE_RAMP=val          - Set disable ramp delay (ms)"));
  Serial.println(F("SET SPEED_RAMP=val            - Set speed ramp factor (0.1-1.0)"));
  Serial.println(F("SET SETTLE_DELAY=val          - Set motion settle delay (ms)"));
  Serial.println(F("--- LED Control Commands ---"));
  Serial.println(F("SET LED_IDLE=val              - Set idle LED period (ms)"));
  Serial.println(F("SET LED_EXTEND=val            - Set extend LED period (ms)"));
  Serial.println(F("SET LED_RETRACT=val           - Set retract LED period (ms)"));
  Serial.println(F("SET LED_ERROR=val             - Set error LED period (ms)"));
  Serial.println(F("SET LED_DEBUG=val             - Set debug LED period (ms)"));
  Serial.println(F("LED_INFO                      - Show LED system information"));
  Serial.println(F("LED_ON                        - Enable LED indicator"));
  Serial.println(F("LED_OFF                       - Disable LED indicator"));
  Serial.println(F("LED_TEST                      - Test all LED states"));
  Serial.println(F("--- General Commands ---"));
  Serial.println(F("SHOW                          - Show current configuration"));
  Serial.println(F("SAVE                          - Save current config to EEPROM"));
  Serial.println(F("LOAD                          - Load config from EEPROM"));
  Serial.println(F("RESET                         - Reset config to defaults"));
  Serial.println(F("MODE                          - Toggle NORMAL/TESTING mode"));
  Serial.println(F("DEBUG                         - Toggle debug status output"));
  Serial.println(F("DEBOUNCE_INFO                 - Show debounce settings and readings"));
  if (operationMode == MODE_TESTING) {
    Serial.println(F("--- Testing Mode Commands ---"));
    Serial.println(F("SENSOR_HIGH                   - Set simulated sensor to HIGH"));
    Serial.println(F("SENSOR_LOW                    - Set simulated sensor to LOW"));
  }
  Serial.println(F("HELP                          - Show this help"));
  Serial.println(F("============================"));
}