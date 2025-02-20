#include "PIDSimulator.h"

PIDSimulator simulator(0.8, 0.3, 2.5, &Serial);

unsigned long lastSetpointChange = 0;
int setpointPhase = 0;

void setup() {
  Serial.begin(9600);

  simulator.setTunings(20.0, 5.0, 8.5);
  simulator.setOvershootParams(0.012, 0.025, 0.975, 0.7);
  simulator.setDampingParams(0.985, 0.88);
  simulator.setNoiseAmplitude(0.0008);

  simulator.setSetpointRange(0.0, 5.5);
  simulator.setOutputLimits(0.0, 500.0);

  simulator.enableSerialInput(true);
  simulator.setup();
}

void loop() {
  simulator.update(200, false);
  simulator.checkSerialInput();

  double setpoint = simulator.getCurrentSetpoint();
  double input = simulator.getCurrentInput();
  double error = setpoint - input;

  // Hitung output dengan respons yang lebih halus
  double output;
  if (error > 0) {
    // Normalisasi error dengan setpoint maksimum (5.5)
    double normalizedError = error / 5.5;
    // Tingkatkan respon dengan mengurangi konstanta exponential
    output = 500.0 * (1.0 - exp(-3.0 * normalizedError));
  } else if (error < 0) {
    output = 0.0;
  } else {
    output = 0.0;
  }

  // Kurangi smoothing factor agar lebih responsif
  static double lastOutput = 0;
  double smoothingFactor = 0.3;  // Naikkan dari 0.15 ke 0.3
  output = (smoothingFactor * output) + ((1 - smoothingFactor) * lastOutput);
  lastOutput = output;

  output = constrain(output, 0, 500);
  output /= 10;

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