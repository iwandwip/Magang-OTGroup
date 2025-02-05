void usbSerialReceiveCallback(const String& dataRecv) {
  String dataHeader = usbSerial.getStrData(dataRecv, 0, "#");
  String dataValue = usbSerial.getStrData(dataRecv, 1, "#");
  dataHeader.toUpperCase();
  if (dataHeader == "DELAY") {  // DELAY#700
    solenoidDelayMs = dataValue.toInt();
    eeprom.writeInt(SOLENOID_DELAY_ADDRESS, solenoidDelayMs);
    Serial.print("| solenoidDelayMs: ");
    Serial.print(solenoidDelayMs);
  } else if (dataHeader == "PULSE") {  // PULSE#800
    solenoidPulseMs = dataValue.toInt();
    eeprom.writeInt(SOLENOID_PULSE_ADDRESS, solenoidPulseMs);
    Serial.print("| solenoidPulseMs: ");
    Serial.print(solenoidPulseMs);
  } else if (dataHeader == "WAIT") {  // WAIT#1000
    solenoidWaitMs = dataValue.toInt();
    eeprom.writeInt(SOLENOID_WAIT_ADDRESS, solenoidWaitMs);
    Serial.print("| solenoidWaitMs: ");
    Serial.print(solenoidWaitMs);
  }
  Serial.println();
}