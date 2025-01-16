#include <ModbusMaster.h>
#include <SoftwareSerial.h>

const int pinRo = 10;  // RX
const int pinRe = 11;  // EN
const int pinDe = 11;  // EN
const int pinDi = 12;  // TX

SoftwareSerial modbus(pinRo, pinDi);
ModbusMaster node;

union FloatToRegister {
  float floatValue;
  uint16_t word[2];
};

void setup() {
  Serial.begin(9600);
  modbus.begin(9600);
  node.begin(1, modbus);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(pinRe, LOW);
  digitalWrite(pinDe, LOW);

  node.preTransmission([]() {
    digitalWrite(pinRe, HIGH);
    digitalWrite(pinDe, HIGH);
  });
  node.postTransmission([]() {
    digitalWrite(pinRe, LOW);
    digitalWrite(pinDe, LOW);
  });
}

void loop() {
  // uint8_t result;

  // result = node.readCoils(5, 1);
  // if (result != node.ku8MBSuccess) {
  //   Serial.print("| result read: 0x");
  //   Serial.print(result, HEX);
  //   Serial.println();
  // }
  // node.clearResponseBuffer();

  const uint8_t dataLen = 3;
  float values[dataLen];
  for (int i = 0; i < dataLen; i++) {
    values[i] = 25.26f + random(0, 10) * 0.1;
  }
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  writeFloatToMultipleHoldingRegister(values, 1, dataLen);
  // node.clearTransmitBuffer();

  // uint32_t dataSend = random(0, 10000);
  // modbus.println(dataSend);
  // if (modbus.available()) {
  //   Serial.println(modbus.readStringUntil('\n'));
  // }
  // Serial.println(dataSend);
}

bool writeFloatToHoldingRegister(float value, uint16_t address) {
  FloatToRegister converter;
  converter.floatValue = value;
  node.setTransmitBuffer(0, converter.word[0]);
  node.setTransmitBuffer(1, converter.word[1]);
  uint8_t result = node.writeMultipleRegisters(address, 2);
  return (result == node.ku8MBSuccess);
}

bool writeFloatToMultipleHoldingRegister(float* values, uint16_t address, uint8_t count) {
  if (count == 0) return false;
  uint8_t transmitIndex = 0;
  FloatToRegister converter;
  for (uint8_t i = 0; i < count; i++) {
    converter.floatValue = values[i];
    node.setTransmitBuffer(transmitIndex++, converter.word[0]);
    node.setTransmitBuffer(transmitIndex++, converter.word[1]);
  }
  uint8_t result = node.writeMultipleRegisters(address, count * 2);
  Serial.println(result, HEX);
  return (result == node.ku8MBSuccess);
}

float readFloatFromHoldingRegister(uint16_t address) {
  FloatToRegister converter;
  uint8_t result = node.readHoldingRegisters(address, 2);
  if (result == node.ku8MBSuccess) {
    converter.word[0] = node.getResponseBuffer(0);
    converter.word[1] = node.getResponseBuffer(1);
    return converter.floatValue;
  }
  return 0.0;
}