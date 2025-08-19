// TEMPORARY DIAGNOSTIC CODE - Tambahkan di loop() ARM2 Control

void loop() {
  // ===== TAMBAHKAN INI DI AWAL LOOP =====
  static unsigned long lastArmDiagnostic = 0;
  if (millis() - lastArmDiagnostic >= 2000) {  // Every 2 seconds
    printArmControlDiagnostic();
    lastArmDiagnostic = millis();
  }
  // ===== END DIAGNOSTIC =====

  processSerialCommands();
  processUSBCommands();
  updateStateMachine();
  updateCommandSequence();
  updateParkSequence();
}

// TAMBAHKAN FUNCTION INI DI BAWAH
void printArmControlDiagnostic() {
  Serial.println("========== ARM2 CONTROL DIAGNOSTIC ==========");
  
  // Device detection
  Serial.print("A4 pin: ");
  Serial.print(digitalRead(A4));
  Serial.print(", A5 pin: ");
  Serial.print(digitalRead(A5));
  Serial.print(", isARM2(): ");
  Serial.print(isARM2());
  Serial.print(", isARM2_device: ");
  Serial.println(isARM2_device);
  
  // Current state
  Serial.print("Current state: ");
  Serial.print(currentState);
  Serial.print(" (");
  switch(currentState) {
    case STATE_ZEROING: Serial.print("ZEROING"); break;
    case STATE_SLEEPING: Serial.print("SLEEPING"); break;
    case STATE_READY: Serial.print("READY"); break;
    case STATE_RUNNING: Serial.print("RUNNING"); break;
    default: Serial.print("UNKNOWN"); break;
  }
  Serial.println(")");
  
  // Pin states
  Serial.print("COMMAND_ACTIVE_PIN(13): ");
  Serial.print(digitalRead(COMMAND_ACTIVE_PIN));
  Serial.print(", MOTOR_DONE_PIN(3): ");
  Serial.println(digitalRead(MOTOR_DONE_PIN));
  
  // Motor ready check
  Serial.print("isMotorReady(): ");
  Serial.println(isMotorReady());
  
  // Sequence status
  Serial.print("currentSequence: ");
  Serial.print(currentSequence);
  Serial.print(", isParkSequenceActive: ");
  Serial.println(isParkSequenceActive);
  
  // Expected device prefix
  const char* devicePrefix = isARM2_device ? "R" : "L";
  Serial.print("Expected command prefix: ");
  Serial.println(devicePrefix);
  
  Serial.println("============================================");
}