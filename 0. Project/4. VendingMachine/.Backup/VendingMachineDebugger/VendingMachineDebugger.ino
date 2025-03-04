#include "AndroidBridge.h"
#include "DriverBridge.h"
#include "AltSoftSerial.h"

AltSoftSerial altSerial;

AndroidBridge android(&Serial, nullptr);
DriverBridge driver(&altSerial, &Serial);

int number = 0;

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
  android.writeOTAddress("OT0001");
}

void loop() {
  if (Serial.available()) {
    int nums = Serial.readStringUntil('\n').toInt();
    if (nums == 0) Serial.println();
    else number = nums;
  }
  driver.receiveMotorResponse();

  // android.handleCommands();

  // if (android.isUnlocking()) driver.openLock();

  // int command = android.getReceivedCommand();
  // if (command >= 0) {
  //   bool status = driver.executeMotorCommand(command);
  //   if (status) android.sendResponse("a");
  //   else android.sendResponse("b");
  // }
}