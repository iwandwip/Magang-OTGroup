#include "Header.h"

void setup() {
  usbSerial.begin(&Serial, 9600);
  solenoidDelayMs = eeprom.readInt(SOLENOID_DELAY_ADDRESS);
  solenoidPulseMs = eeprom.readInt(SOLENOID_PULSE_ADDRESS);
  solenoidWaitMs = eeprom.readInt(SOLENOID_WAIT_ADDRESS);

  Serial.print("| solenoidDelayMs: ");
  Serial.print(solenoidDelayMs);
  Serial.print("| solenoidPulseMs: ");
  Serial.print(solenoidPulseMs);
  Serial.print("| solenoidWaitMs: ");
  Serial.print(solenoidWaitMs);
  Serial.println();
}

void loop() {
  switch (systemState) {
    case SYSTEM_IDLE:
      proximityState = proximity.getStateRaw();
      if (proximityState) {
        ledBuiltIn.off();
      } else {
        ledBuiltIn.on();
        solenoidTimer.setDuration(solenoidDelayMs);
        solenoidTimer.reset();
        solenoidTimer.start();
        systemState = SYSTEM_RUN;
      }
      solenoid.off();
      break;
    case SYSTEM_RUN:
      if (solenoidTimer.isExpired()) {
        solenoid.on();
        delay(solenoidPulseMs);
        solenoid.off();
        solenoidTimer.setDuration(solenoidWaitMs);
        solenoidTimer.reset();
        solenoidTimer.start();
        systemState = SYSTEM_WAIT;
      }
      break;
    case SYSTEM_WAIT:
      if (!solenoidTimer.isExpired()) ledBuiltIn.toggleAsync(25);
      else {
        systemState = SYSTEM_IDLE;
        arduinoReset();
      }
      break;
  };

  usbSerial.receiveString(usbSerialReceiveCallback);

  DigitalIn::updateAll(&proximity, DigitalIn::stop());
  DigitalOut::updateAll(&solenoid, &ledBuiltIn, DigitalOut::stop());
}
