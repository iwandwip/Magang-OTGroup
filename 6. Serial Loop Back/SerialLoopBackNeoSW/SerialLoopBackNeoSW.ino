#include <NeoSWSerial.h>

NeoSWSerial neoSerial(11, 10);  // RX, TX

void setup() {
  Serial.begin(9600);
  neoSerial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    neoSerial.println(Serial.readStringUntil('\n'));
  }

  if (neoSerial.available()) {
    Serial.println(neoSerial.readStringUntil('\n'));
  }
}
