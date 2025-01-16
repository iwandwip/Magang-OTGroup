uint16_t* floatToRegister(float value) {
  static uint16_t registerWords[2];
  FloatToRegister converter;
  converter.floatValue = value;
  registerWords[0] = converter.word[0];
  registerWords[1] = converter.word[1];
  return registerWords;
}

float registerToFloat(uint16_t* registers) {
  FloatToRegister converter;
  converter.word[0] = registers[0];
  converter.word[1] = registers[1];
  return converter.floatValue;
}