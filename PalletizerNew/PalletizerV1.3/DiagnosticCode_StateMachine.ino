// TEMPORARY DIAGNOSTIC CODE - Tambahkan di loop() State Machine

void loop() {
  unsigned long currentTime = millis();

  // ===== TAMBAHKAN INI DI AWAL LOOP =====
  static unsigned long lastDiagnostic = 0;
  if (currentTime - lastDiagnostic >= 1000) {  // Every 1 second
    printFullDiagnostic();
    lastDiagnostic = currentTime;
  }
  // ===== END DIAGNOSTIC =====

  // Read sensors
  if (currentTime - lastSensorRead >= SENSOR_READ_INTERVAL) {
    readSensors();
    lastSensorRead = currentTime;
  }

  // ... rest of existing loop code
}

// TAMBAHKAN FUNCTION INI DI BAWAH
void printFullDiagnostic() {
  Serial.println("========== FULL SYSTEM DIAGNOSTIC ==========");
  
  // Hardware pin readings
  Serial.print("Hardware pins - ARM1_PIN(7): ");
  Serial.print(digitalRead(ARM1_PIN));
  Serial.print(", ARM2_PIN(8): ");
  Serial.println(digitalRead(ARM2_PIN));
  
  // ARM state machine status
  Serial.print("ARM1 State: ");
  Serial.print(getStateString(arm1_sm.state));
  Serial.print(", is_busy: ");
  Serial.print(arm1_sm.is_busy);
  Serial.print(", need_special: ");
  Serial.println(arm1_sm.need_special_command);
  
  Serial.print("ARM2 State: ");
  Serial.print(getStateString(arm2_sm.state));
  Serial.print(", is_busy: ");
  Serial.print(arm2_sm.is_busy);
  Serial.print(", need_special: ");
  Serial.println(arm2_sm.need_special_command);
  
  // ARM readiness calculation
  bool arm1_ready = (arm1_sm.state == ARM_IDLE) && !arm1_sm.is_busy && !arm1_sm.need_special_command;
  bool arm2_ready = (arm2_sm.state == ARM_IDLE) && !arm2_sm.is_busy && !arm2_sm.need_special_command;
  
  Serial.print("ARM1 ready: ");
  Serial.print(arm1_ready ? "YES" : "NO");
  Serial.print(", ARM2 ready: ");
  Serial.println(arm2_ready ? "YES" : "NO");
  
  // System variables
  Serial.print("arm_in_center: ");
  Serial.print(arm_in_center);
  Serial.print(", last_arm_sent: ");
  Serial.println(last_arm_sent);
  
  // Sensor states
  Serial.print("Sensors - S1: ");
  Serial.print(sensor1_state);
  Serial.print(", S2: ");
  Serial.print(sensor2_state);
  Serial.print(", S3: ");
  Serial.println(sensor3_state);
  
  // ARM positions
  Serial.print("ARM1 pos: ");
  Serial.print(arm1_sm.current_pos);
  Serial.print("/");
  Serial.print(arm1_sm.total_commands);
  Serial.print(", ARM2 pos: ");
  Serial.print(arm2_sm.current_pos);
  Serial.print("/");
  Serial.println(arm2_sm.total_commands);
  
  Serial.println("============================================");
}