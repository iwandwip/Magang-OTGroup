void serialDebugging() {
  Serial.print("| adc: ");
  Serial.print(writeHoldingRegister[ADC_REGISTER]);
  Serial.print("| v: ");
  Serial.print(writeHoldingRegister[VOLT_REGISTER]);
  Serial.print("| do: ");
  Serial.print(writeHoldingRegister[DO_REGISTER]);
  Serial.print("| out: ");
  Serial.print(writeHoldingRegister[OUT_FREQUENCY_REGISTER]);
  Serial.print("| pwm: ");
  Serial.print(writeHoldingRegister[PWM_OUT_REGISTER]);

  Serial.print("| cal: ");
  Serial.print(writeHoldingRegister[CALIBRATION_REGISTER]);
  Serial.print("| in: ");
  Serial.print(writeHoldingRegister[IN_FREQUENCY_REGISTER]);
  Serial.print("| thresh: ");
  Serial.print(writeHoldingRegister[DO_THRESHOLD_REGISTER]);
  Serial.print("| above: ");
  Serial.print(writeHoldingRegister[ABOVE_THESHOLD_REGISTER]);
  Serial.print("| below: ");
  Serial.print(writeHoldingRegister[BELOW_THESHOLD_REGISTER]);
  Serial.print("| T: ");
  Serial.print(writeHoldingRegister[TRANSITION_TIME_REGISTER]);

  Serial.println();
}