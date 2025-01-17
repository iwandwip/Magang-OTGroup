#include <AltSoftSerial.h>

// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10
// Arduino Leonardo   5        13       (none)
// Arduino Mega      46        48       44, 45

AltSoftSerial altSerial;

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    altSerial.println(Serial.readStringUntil('\n'));
  }

  if (altSerial.available()) {
    Serial.println(altSerial.readStringUntil('\n'));
  }
}