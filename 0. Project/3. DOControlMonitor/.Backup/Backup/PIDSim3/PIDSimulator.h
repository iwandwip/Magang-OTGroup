#ifndef PID_SIMULATOR_H
#define PID_SIMULATOR_H

#include "Arduino.h"
#include "PID_v1.h"

class PIDSimulator {
private:
  double MIN_SETPOINT;
  double MAX_SETPOINT;
  double MIN_OUTPUT;
  double MAX_OUTPUT;

  double MOMENTUM_GAIN;
  double INITIAL_MOMENTUM;
  double MOMENTUM_DAMPING;
  double MAX_MOMENTUM;
  double NEAR_TARGET_DAMPING;
  double AT_TARGET_DAMPING;

  double Setpoint;
  double Input;
  double Output;
  double Kp, Ki, Kd;
  PID* pid;

  double simulatedValue;
  double momentum;
  double noiseAmplitude;

  String inputString;
  bool stringComplete;

  bool serialInputEnabled;
  bool hasSerialPort;
  Stream* serial;

  const double EXP_FACTOR = -3.0;
  const double SMOOTHING_FACTOR = 0.3;
  double lastSmoothOutput;

  void simulateInput();
  void sendDataToPlotter();

public:
  PIDSimulator(double kp = 2, double ki = 5, double kd = 1, Stream* serialPort = nullptr);
  ~PIDSimulator();

  void setup();
  void update(unsigned long interval = 50, bool plot = false);
  void checkSerialInput();

  void setTunings(double kp, double ki, double kd);
  void setKp(double kp);
  void setKi(double ki);
  void setKd(double kd);
  void setSetpointRange(double minSP, double maxSP);
  void setOutputLimits(double minOutput, double maxOutput);
  void setOvershootParams(double gain, double initial, double damping, double maxMom);
  void setDampingParams(double nearTarget, double atTarget);
  void setNoiseAmplitude(double amplitude);
  void setSetpoint(double setpoint);
  void enableSerialInput(bool enable);
  void setSerialPort(Stream* serialPort);

  double getCurrentSetpoint();
  double getCurrentInput();
  double getCurrentOutput();
  double getSmoothOutput();
  double getMinOutput();
  double getMaxOutput();
  double getKp();
  double getKi();
  double getKd();
  bool isSerialInputEnabled();

  static double mapDouble(double x, double in_min, double in_max, double out_min, double out_max);
  static float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
};

#endif