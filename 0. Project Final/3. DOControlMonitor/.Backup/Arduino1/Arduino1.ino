#define ENABLE_MODULE_MODBUS
#define ENABLE_MODULE_DIGITAL_OUTPUT

#include "Kinematrix.h"

const int pinRo = 10;
const int pinRe = 11;
const int pinDe = 11;
const int pinDi = 12;

modbusDevice regBank;
modbusSlave slave;

DigitalOut ledBuiltin(LED_BUILTIN);

SoftwareSerial modbus(pinRo, pinDi);

void setup() {
  Serial.begin(9600);
  pinMode(pinRe, OUTPUT);
  // pinMode(pinDe, OUTPUT);

  digitalWrite(pinRe, LOW);
  digitalWrite(pinDe, LOW);

  regBank.setId(1);
  regBank.add(1);
  regBank.add(2);
  regBank.add(3);

  regBank.add(40001);  // sensor value float[0]
  regBank.add(40002);  // sensor value float[1]
  regBank.add(40003);  // sensor value
  regBank.add(40004);  // sensor value

  slave._device = &regBank;
  slave.setBaud(&modbus, 9600);
}

void loop() {
  // regBank.sendDataFloat(random(0, 1000) * 0.1, 40001);

  int button1 = regBank.get(1);
  int button2 = regBank.get(2);
  int button3 = regBank.get(3);

  // Serial.print("| button1: ");
  // Serial.print(button1);
  // Serial.print("| button2: ");
  // Serial.print(button2);
  // Serial.print("| button3: ");
  // Serial.print(button3);
  // Serial.println();

  // if (modbus.available()) {
  //   ledBuiltin.on();
  //   delay(1000);
  //   ledBuiltin.off();
  // }

  slave.run([]() {
    ledBuiltin.toggle();
  });

  // ledBuiltin.toggleAsync(100);
  ledBuiltin.update();
}
