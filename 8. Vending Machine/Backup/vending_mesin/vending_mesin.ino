#include <AltSoftSerial.h>
#include "vending_data.h"

// Board          Transmit  Receive   PWM Unusable
// -----          --------  -------   ------------
// Arduino Uno        9         8         10

AltSoftSerial altSerial;

int dataSize = sizeof(hexData00) / sizeof(hexData00[0]);

void setup() {
  Serial.begin(9600);
  altSerial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (Serial.available()) {
    while (Serial.available()) {
      Serial.read();
    }

    for (int i = 0; i < hexSize; i++) {
      Serial.print("| sent[" + String(i + 11) + "]: ");
      for (int j = 0; j < 20; j++) {
        altSerial.write(hexDataArr[i][j]);
        Serial.print(hexDataArr[i][j], HEX);
      }
      Serial.println();
      delay(8000);
    }
  }

  //   // for (int j = 0; j < 15; j++) {
  //   //   Serial.print("| check: ");
  //   //   for (int i = 0; i < dataSize; i++) {
  //   //     altSerial.write(checkData[i]);
  //   //     Serial.print(checkData[i], HEX);
  //   //   }
  //   //   Serial.println();

  //   //   unsigned long startTime = millis();
  //   //   while (millis() - startTime < 1000) {
  //   //     if (altSerial.available()) {
  //   //       Serial.print("| recv: ");
  //   //       while (altSerial.available()) {
  //   //         Serial.print(altSerial.read(), HEX);
  //   //       }
  //   //       Serial.println();
  //   //       break;
  //   //     }
  //   //   }
  //   //   //
  //   // }
  // }

  // int status = altSerial.available();
  // Serial.print("| status: ");
  // Serial.print(status);
  // Serial.println();

  // if (!altSerial.available()) {
  //   static uint32_t ledTimer;
  //   if (millis() - ledTimer >= 75) {
  //     ledTimer = millis();
  //     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  //   }
  // } else {
  //   digitalWrite(LED_BUILTIN, HIGH);
  //   Serial.print("| recv: ");
  //   while (altSerial.available()) {
  //     Serial.print(altSerial.read(), HEX);
  //   }
  //   Serial.println();
  // }

  // if (altSerial.available()) {
  //   Serial.print("| recv: ");
  //   while (altSerial.available()) {
  //     Serial.print(altSerial.read(), HEX);
  //   }
  //   Serial.println();
}