#include <ModbusMaster.h>
#include <SoftwareSerial.h>

const int pinRo = 10;  // RX
const int pinRe = 11;  // EN
const int pinDe = 11;  // EN
const int pinDi = 12;  // TX

SoftwareSerial modbus(pinRo, pinDi);
ModbusMaster node;

void preTransmission() {
  digitalWrite(pinRe, HIGH);
  digitalWrite(pinDe, HIGH);
}

void postTransmission() {
  digitalWrite(pinRe, LOW);
  digitalWrite(pinDe, LOW);
}

void setup() {
  Serial.begin(9600);
  modbus.begin(9600);
  node.begin(1, modbus);  // Slave ID HMI

  pinMode(pinRe, OUTPUT);
  pinMode(pinDe, OUTPUT);

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
}

void loop() {
  uint8_t result;

  // float values[] = { random(0, 1000) * 0.1, random(0, 1000) * 0.1, random(0, 1000) * 0.1, random(0, 1000) * 0.1 };
  // if (writeMultipleFloatsToRegisters(values, 1, 4)) {  // Mulai dari register 41, 2 nilai float (4 register)
  //   Serial.println("Write multiple floats successful");
  // } else {
  //   Serial.println("Write failed");
  // }

  // Serial.println(readSingleRegisterAsFloat(101));

  bool coilStatus = readSingleCoil(2);
  Serial.print("Coil 0 status: ");
  Serial.println(coilStatus);
}

float readSingleRegisterAsFloat(uint16_t address) {
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  uint8_t result = node.readHoldingRegisters(address, 2);
  if (result == node.ku8MBSuccess) {
    converter.word[0] = node.getResponseBuffer(0);
    converter.word[1] = node.getResponseBuffer(1);
    return converter.floatValue;
  } else {
    return NAN;
  }
}

float convertRegistersToFloat(uint16_t registerHigh, uint16_t registerLow) {
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  converter.word[0] = registerLow;
  converter.word[1] = registerHigh;
  return converter.floatValue;
}

bool writeMultipleFloatsToRegisters(float* values, uint16_t startAddress, uint8_t count) {
  if (count == 0) return false;
  union {
    float floatValue;
    uint16_t word[2];
  } converter;
  uint8_t transmitIndex = 0;
  for (uint8_t i = 0; i < count; i++) {
    converter.floatValue = values[i];
    node.setTransmitBuffer(transmitIndex++, converter.word[0]);
    node.setTransmitBuffer(transmitIndex++, converter.word[1]);
  }
  uint8_t result = node.writeMultipleRegisters(startAddress, count * 2);
  return (result == node.ku8MBSuccess);
}
