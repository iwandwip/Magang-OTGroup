#define ENABLE_MODULE_DIGITAL_OUTPUT
#include "Kinematrix.h"

enum LedIndicator {
  LED_GREEN = 0,
  LED_YELLOW = 1,
  LED_RED = 2,
  LED_OFF = 3,
};

const int MAX_LED_INDICATOR_SIZE = 3;
DigitalOut ledIndicator[MAX_LED_INDICATOR_SIZE] = {
  DigitalOut(4, true),
  DigitalOut(5, true),
  DigitalOut(6, true),
};

void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.available()) {
    int numLeds = Serial.readStringUntil('\n').toInt();
    Serial.print("| numLeds: ");
    Serial.print(numLeds);
    if (numLeds >= MAX_LED_INDICATOR_SIZE) return;
    ledIndicator[numLeds].toggle();
    Serial.print("| getState(): ");
    Serial.print(ledIndicator[numLeds].getState());
    Serial.println();
  }
  for (int i = 0; i < MAX_LED_INDICATOR_SIZE; i++) {
    ledIndicator[i].update();
  }
}
