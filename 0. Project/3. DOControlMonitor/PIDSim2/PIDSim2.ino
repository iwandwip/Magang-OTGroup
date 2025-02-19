#include "PIDSimulator.h"

PIDSimulator simulator(0.8, 0.3, 2.5, &Serial);

unsigned long lastSetpointChange = 0;
int setpointPhase = 0;

void setup() {
  Serial.begin(9600);
  simulator.setTunings(0.8, 0.3, 2.5);
  simulator.setOvershootParams(0.015, 0.03, 0.97, 0.8);
  simulator.setDampingParams(0.98, 0.85);
  simulator.setNoiseAmplitude(0.001);
  simulator.setSetpointRange(0.0, 5.5);

  simulator.enableSerialInput(true);
  simulator.setup();
}

void loop() {
  simulator.update(50);
  simulator.checkSerialInput();

  /*
  if (millis() - lastSetpointChange > 12000) {
    lastSetpointChange = millis();
    
    switch (setpointPhase) {
      case 0: simulator.setSetpoint(0.1); break;  // Almost zero
      case 1: simulator.setSetpoint(5.0); break;  // Near max
      case 2: simulator.setSetpoint(0.0); break;  // Zero
      case 3: simulator.setSetpoint(2.5); break;  // Mid-range
    }
    setpointPhase = (setpointPhase + 1) % 4;
  }
  */
}