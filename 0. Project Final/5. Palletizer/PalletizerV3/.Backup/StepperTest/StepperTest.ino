#include "StepperFunctionTest.h"

#define CLK_PIN 10    // Step pulse output
#define CW_PIN 11     // Direction output
#define EN_PIN 12     // Enable output
#define SENSOR_PIN 6  // Homing sensor input

StepperFunctionTest stepperTest(CLK_PIN, CW_PIN, EN_PIN, SENSOR_PIN);

void setup() {
  stepperTest.begin();

  Serial.println("StepperFunctionTest ready");
  Serial.println("Enter commands via Serial monitor:");
  Serial.println("  cw,1000,500    - Move 1000 steps clockwise at 500 steps/sec");
  Serial.println("  ccw,1000,500   - Move 1000 steps counter-clockwise at 500 steps/sec");
  Serial.println("  home           - Run homing sequence");
  Serial.println("  back_forth,5,1000,500 - 5 cycles of 1000 steps at 500 steps/sec");
  Serial.println("  sequence,500,500 - Run test sequence with 500 steps at 500 steps/sec");
  Serial.println("  stop           - Stop all motion");
  Serial.println("  status         - Print current status");
}

void loop() {
  stepperTest.update();
}