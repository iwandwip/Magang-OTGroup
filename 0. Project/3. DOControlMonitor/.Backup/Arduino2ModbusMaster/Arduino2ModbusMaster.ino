#include <ModbusMaster.h>
#include <SoftwareSerial.h>

const int pinRo = 10;
const int pinRe = 11;
const int pinDe = 11;
const int pinDi = 12;

ModbusMaster node;
SoftwareSerial modbus(pinRo, pinDi);

void setup() {
  Serial.begin(9600);
  modbus.begin(9600);
  node.begin(1, modbus);
}

void loop() {
  static uint32_t i;
  uint8_t j, result;
  uint16_t data[6];

  i++;

  // set word 0 of TX buffer to least-significant word of counter (bits 15..0)
  node.setTransmitBuffer(0, lowWord(i));

  // set word 1 of TX buffer to most-significant word of counter (bits 31..16)
  node.setTransmitBuffer(1, highWord(i));

  // slave: write TX buffer to (2) 16-bit registers starting at register 0
  result = node.writeMultipleRegisters(0, 2);

  // slave: read (6) 16-bit registers starting at register 2 to RX buffer
  result = node.readHoldingRegisters(2, 6);

  // do something with data if read is successful
  if (result == node.ku8MBSuccess) {
    for (j = 0; j < 6; j++) {
      data[j] = node.getResponseBuffer(j);
    }
  }
}