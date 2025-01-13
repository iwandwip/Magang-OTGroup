void debug() {
  // Serial.print("| volt: ");
  // Serial.print(wlcSensor.voltage);
  Serial.print("| raw: ");
  Serial.print(wlcSensor.raw);
  Serial.print("| state: ");
  Serial.print(wlcSensor.state);
  Serial.print("| radar: ");
  Serial.print(radarSensor);
  Serial.print("| currentState: ");
  if (currentState == SystemState::WAITING) {
    Serial.print("SystemState::WAITING");
    if (!wlcSensor.isTimerStarted) {
      Serial.print("| thresholdStartTime: ");
      Serial.print(0);
    } else {
      Serial.print("| thresholdStartTime: ");
      Serial.print(uint32_t(millis() - thresholdStartTime) / 1000);
    }
  } else if (currentState == SystemState::FILLING) {
    Serial.print("SystemState::FILLING");
    Serial.print("| fillingStartTime: ");
    Serial.print(uint32_t(millis() - fillingStartTime) / 1000);  // 250
    Serial.print("| Limit: 250");
  } else if (currentState == SystemState::TRANSFER) {
    Serial.print("SystemState::TRANSFER");
    Serial.print("| transferStartTime: ");
    Serial.print(uint32_t(millis() - transferStartTime) / 1000);  //380
    Serial.print("| Limit: 380");
  }
  Serial.println();
}

void handleTesting() {
  int data = Serial.readStringUntil('\n').toInt();
  if (data == 1) {
    digitalWrite(PIN_FILLING_PUMP, !digitalRead(PIN_FILLING_PUMP));
    digitalWrite(PIN_DOSING_PUMP, !digitalRead(PIN_DOSING_PUMP));
  } else if (data == 2) {
    digitalWrite(PIN_DOSING_PUMP, !digitalRead(PIN_DOSING_PUMP));
  } else if (data == 3) {
    digitalWrite(PIN_TRANSFER_PUMP, !digitalRead(PIN_TRANSFER_PUMP));
  }
}