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