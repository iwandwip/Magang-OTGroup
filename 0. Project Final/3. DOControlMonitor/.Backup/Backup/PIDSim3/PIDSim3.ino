#include "PIDSimulator.h"

PIDSimulator simulator(0.8, 0.3, 2.5, &Serial);

unsigned long lastSetpointChange = 0;
int setpointPhase = 0;

void setup() {
  Serial.begin(9600);

  simulator.setTunings(25.0, 8.0, 6.0);
  simulator.setOvershootParams(0.01, 0.02, 0.99, 0.65);
  simulator.setDampingParams(0.995, 0.92);
  simulator.setNoiseAmplitude(0.0005);

  simulator.setSetpointRange(0.0, 5.5);
  simulator.setOutputLimits(20.0, 50.0);

  simulator.enableSerialInput(true);
  simulator.setup();
}

void loop() {
  simulator.update(30000, false);
  simulator.checkSerialInput();

  double setpoint = abs(simulator.getCurrentSetpoint());
  double input = abs(simulator.getCurrentInput());
  double output = abs(simulator.getSmoothOutput());

  Serial.print("Lower:");
  Serial.print(-50 * 1.25, 2);
  Serial.print(" Upper:");
  Serial.print(50 * 1.25, 2);

  Serial.print(" setpoint:");
  Serial.print(setpoint, 2);
  Serial.print(" input:");
  Serial.print(input, 2);
  Serial.print(" output:");
  Serial.print(output, 2);
  Serial.println();
}