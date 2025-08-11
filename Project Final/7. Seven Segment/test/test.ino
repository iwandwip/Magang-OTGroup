#define ENABLE_MODULE_SEVEN_SEGMENT_RAW
#include "Kinematrix.h"

const uint8_t digitPins[4] = { 12, 9, 8 };
SevenSegmentRaw display(11, 7, 5, 3, 2, 10, 6, 4, (uint8_t*)digitPins, 3, SEGMENT_ACTIVE_LOW, DIGIT_ACTIVE_HIGH);

void setup() {
  Serial.begin(9600);
  display.begin();
  display.setBrightness(100);
  display.setPaddingMode(PADDING_BLANKS);
  // display.testDigits(600, 5);
}

void loop() {
  static uint8_t number = 0;
  display.displayNumber(number);
  number++;
  delay(100);
}
