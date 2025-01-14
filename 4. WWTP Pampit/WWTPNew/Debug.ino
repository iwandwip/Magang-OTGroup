void debug() {
  // Serial.print("| volt: ");
  // Serial.print(wlcSensor.voltage);
  Serial.print("| adc: ");
  Serial.print(wlcSensor.adc);
  // Serial.print("| raw: ");
  // Serial.print(wlcSensor.raw);
  // Serial.print("| state: ");
  // Serial.print(wlcSensor.state);
  Serial.print("| radar: ");
  Serial.print(radarSensor);
  Serial.print("| state: ");
  if (currentState == SystemState::WAITING) {
    Serial.print("WAITING");
    if (!wlcSensor.isTimerStarted) {
      Serial.print("| t: ");
      Serial.print(0);
    } else {
      Serial.print("| t: ");
      Serial.print(uint32_t(millis() - thresholdStartTime) / 1000);
    }
  } else if (currentState == SystemState::FILLING) {
    Serial.print("FILLING");
    Serial.print("| t: ");
    Serial.print(uint32_t(millis() - fillingStartTime) / 1000);  // 250
    Serial.print("| Limit: ");
    Serial.print(FILLING_TIMEOUT_S);
    Serial.print("s");
  } else if (currentState == SystemState::TRANSFER) {
    Serial.print("TRANSFER");
    Serial.print("| t: ");
    Serial.print(uint32_t(millis() - transferStartTime) / 1000);  //380
    Serial.print("| Limit: ");
    Serial.print(TRANSFER_TIMEOUT_S);
    Serial.print("s");
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