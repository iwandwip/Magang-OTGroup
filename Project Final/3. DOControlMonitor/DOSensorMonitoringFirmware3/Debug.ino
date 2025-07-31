void serialDebugging() {
  Serial.print("| raw: ");
  Serial.print(analogRead(DO_SENSOR_PIN));
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
  Serial.print(readHoldingRegister[CALIBRATION_REGISTER]);
  Serial.print("| in: ");
  Serial.print(readHoldingRegister[IN_FREQUENCY_REGISTER]);
  Serial.print("| thresh: ");
  Serial.print(readHoldingRegister[DO_THRESHOLD_REGISTER]);
  Serial.print("| above: ");
  Serial.print(readHoldingRegister[ABOVE_THESHOLD_REGISTER]);
  Serial.print("| below: ");
  Serial.print(readHoldingRegister[BELOW_THESHOLD_REGISTER]);
  Serial.print("| T: ");
  Serial.print(readHoldingRegister[TRANSITION_TIME_REGISTER]);

  Serial.print("| kp: ");
  Serial.print(readHoldingRegister[PARAM_KP_PID_REGISTER]);
  Serial.print("| ki: ");
  Serial.print(readHoldingRegister[PARAM_KI_PID_REGISTER]);
  Serial.print("| kd: ");
  Serial.print(readHoldingRegister[PARAM_KD_PID_REGISTER]);
  Serial.print("| sp: ");
  Serial.print(readHoldingRegister[PARAM_SP_PID_REGISTER]);
  Serial.print("| do: ");
  Serial.print(readHoldingRegister[PARAM_DO_QC_INPUT_REGISTER]);
  Serial.print("| cf: ");
  Serial.print(calibrationFactor);

  Serial.print("| Ct: ");
  Serial.print(eeprom.getWriteCount());

  Serial.println();
}