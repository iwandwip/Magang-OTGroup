#include <AltSoftSerial.h>

// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10

AltSoftSerial altSerial;
byte hexData[] = { 0x01, 0x05, 0x00, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x89 };
int dataSize = sizeof(hexData) / sizeof(hexData[0]);

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
  // for (int i = 0; i < dataSize; i++) {
  //   altSerial.print((char)hexData[i]);
  // }
}

void loop() {
  if (Serial.available()) {
    altSerial.println(Serial.readStringUntil('\n'));
  }

  if (altSerial.available()) {
    Serial.println(altSerial.readStringUntil('\n'));
    // Serial.println(altSerial.read(), HEX);
  }
}