// VendingMachine.ino
#include "AndroidBridge.h"
#include "DriverBridge.h"
#include "AltSoftSerial.h"

AltSoftSerial altSerial;

AndroidBridge androidBridge(&Serial, &Serial);
DriverBridge driverBridge(&altSerial, &Serial);

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
}

void loop() {
  androidBridge.handleCommands();

  int command = androidBridge.getReceivedCommand();
  if (command >= 0) {
    driverBridge.executeMotorCommand(command);
  }
}