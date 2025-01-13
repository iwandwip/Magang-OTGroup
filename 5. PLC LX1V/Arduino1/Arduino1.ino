#define ENABLE_MODULE_MODBUS

#include "Kinematrix.h"

modbusDevice registerBank;
modbusSlave slave;

void setup() {
  Serial.begin(9600);

  registerBank.setId(1);
  registerBank.add(1);  // Button Start/Stop
  registerBank.add(2);  // Button Fuzzy/PID

  registerBank.add(10001);  // Add Digital Input registers
  registerBank.add(10002);  // Add Digital Input registers

  registerBank.add(40001);  // sensor value float[0]
  registerBank.add(40002);  // sensor value float[1]
  registerBank.add(40003);  // sensor value
  registerBank.add(40004);  // sensor value

  slave._device = &registerBank;
  slave.setBaud(new SoftwareSerial(2, 3), 9600);
}

void loop() {
  slave.run();
}
