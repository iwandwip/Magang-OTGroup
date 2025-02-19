#include "PIDSimulator.h"

PIDSimulator simulator(0.8, 0.3, 2.5, &Serial);

unsigned long lastSetpointChange = 0;
int setpointPhase = 0;

void setup() {
  Serial.begin(9600);

  simulator.setTunings(0.7, 0.25, 3.2);
  simulator.setOvershootParams(0.012, 0.025, 0.975, 0.7);
  simulator.setDampingParams(0.985, 0.88);
  simulator.setNoiseAmplitude(0.0008);

  simulator.setSetpointRange(0.0, 5.5);
  simulator.setOutputLimits(0.0, 500.0);

  simulator.enableSerialInput(true);
  simulator.setup();
}

void loop() {
  simulator.update(50, false);
  simulator.checkSerialInput();

  double setpoint = abs(simulator.getCurrentSetpoint());
  double input = abs(simulator.getCurrentInput());
  double output = abs(simulator.getCurrentOutput());

  Serial.print("| setpoint: ");
  Serial.print(setpoint);
  Serial.print("| input: ");
  Serial.print(input);
  Serial.print("| output: ");
  Serial.print(output);
  Serial.println();
}