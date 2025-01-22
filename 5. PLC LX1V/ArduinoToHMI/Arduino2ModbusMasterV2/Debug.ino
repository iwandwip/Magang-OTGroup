void serialDebugging() {
  // Serial.print("| adc: ");
  // Serial.print(writeHoldingRegister[ADC_REGISTER]);
  // Serial.print("| V: ");
  // Serial.print(writeHoldingRegister[VOLT_REGISTER]);
  // Serial.print("| DO: ");
  // Serial.print(writeHoldingRegister[DO_REGISTER]);
  // Serial.print("| Out: ");
  // Serial.print(writeHoldingRegister[OUT_FREQUENCY_REGISTER]);

  // Serial.print("| Cal: ");
  // Serial.print(readHoldingRegister[CALIBRATION_REGISTER]);
  // Serial.print("| In: ");
  // Serial.print(readHoldingRegister[IN_FREQUENCY_REGISTER]);
  // Serial.print("| Auto: ");
  // Serial.print(readCoil[AUTO_COIL]);

  // Serial.print("| PWM: ");
  // Serial.print(pwmOutput);

  Serial.print("| DO: ");
  Serial.print(readHoldingRegister[DO_THRESHOLD_REGISTER]);
  Serial.print("| AB: ");
  Serial.print(readHoldingRegister[ABOVE_THESHOLD_REGISTER]);
  Serial.print("| BL: ");
  Serial.print(readHoldingRegister[BELLOW_THESHOLD_REGISTER]);
  Serial.print("| TT: ");
  Serial.print(readHoldingRegister[TRANSITION_TIME_REGISTER]);
  Serial.println();
}