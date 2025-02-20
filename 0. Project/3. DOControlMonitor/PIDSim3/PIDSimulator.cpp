#include "PIDSimulator.h"

PIDSimulator::PIDSimulator(double kp, double ki, double kd, Stream* serialPort) {
  // Parameter default
  MIN_SETPOINT = 0.0;
  MAX_SETPOINT = 100.0;
  MIN_OUTPUT = 0.0;    // Default min output
  MAX_OUTPUT = 255.0;  // Default max output

  MOMENTUM_GAIN = 0.05;
  INITIAL_MOMENTUM = 0.08;
  MOMENTUM_DAMPING = 0.85;
  MAX_MOMENTUM = 3.0;
  NEAR_TARGET_DAMPING = 0.9;
  AT_TARGET_DAMPING = 0.7;

  // Inisialisasi variabel
  Kp = kp;
  Ki = ki;
  Kd = kd;
  Setpoint = 0.0;
  Input = 0.0;
  simulatedValue = 0.0;
  momentum = 0.0;
  noiseAmplitude = 0.5;
  lastSmoothOutput = 0.0; 

  inputString = "";
  stringComplete = false;

  // Set the serial port
  serial = serialPort;
  hasSerialPort = (serial != nullptr);

  // Default: Serial input diaktifkan jika serial port tersedia
  serialInputEnabled = hasSerialPort;

  // Inisialisasi PID
  pid = new PID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);
  pid->SetOutputLimits(MIN_OUTPUT, MAX_OUTPUT);
  pid->SetMode(AUTOMATIC);
}

PIDSimulator::~PIDSimulator() {
  delete pid;
}

void PIDSimulator::setSerialPort(Stream* serialPort) {
  serial = serialPort;
  hasSerialPort = (serial != nullptr);

  // Disable serial input if serial port is null
  if (!hasSerialPort) {
    serialInputEnabled = false;
  }
}

void PIDSimulator::setup() {
  if (!hasSerialPort) {
    return;  // No serial port available, skip setup messages
  }

  if (serialInputEnabled) {
    serial->print("Masukkan nilai Setpoint (");
    serial->print(MIN_SETPOINT);
    serial->print(" - ");
    serial->print(MAX_SETPOINT);
    serial->println("):");
  } else {
    serial->println("Mode setpoint manual aktif. Serial input dinonaktifkan.");
  }
  delay(100);
}

void PIDSimulator::setTunings(double kp, double ki, double kd) {
  Kp = kp;
  Ki = ki;
  Kd = kd;
  pid->SetTunings(Kp, Ki, Kd);
}

void PIDSimulator::setKp(double kp) {
  Kp = kp;
  pid->SetTunings(Kp, Ki, Kd);
}

void PIDSimulator::setKi(double ki) {
  Ki = ki;
  pid->SetTunings(Kp, Ki, Kd);
}

void PIDSimulator::setKd(double kd) {
  Kd = kd;
  pid->SetTunings(Kp, Ki, Kd);
}

double PIDSimulator::getKp() {
  return Kp;
}

double PIDSimulator::getKi() {
  return Ki;
}

double PIDSimulator::getKd() {
  return Kd;
}

void PIDSimulator::setSetpoint(double setpoint) {
  // Validasi nilai setpoint
  if (setpoint >= MIN_SETPOINT && setpoint <= MAX_SETPOINT) {
    // Simpan setpoint sebelumnya
    double oldSetpoint = Setpoint;

    // Update setpoint
    Setpoint = setpoint;

    // Hitung perubahan dan tambahkan momentum
    double change = Setpoint - oldSetpoint;
    momentum += change * INITIAL_MOMENTUM;

    // Nonaktifkan input serial secara otomatis
    serialInputEnabled = false;
  } else if (hasSerialPort) {
    serial->print("Nilai setpoint tidak valid! Harus antara ");
    serial->print(MIN_SETPOINT);
    serial->print(" dan ");
    serial->println(MAX_SETPOINT);
  }
}

void PIDSimulator::setOutputLimits(double minOutput, double maxOutput) {
  if (minOutput < maxOutput) {
    MIN_OUTPUT = minOutput;
    MAX_OUTPUT = maxOutput;
    pid->SetOutputLimits(MIN_OUTPUT, MAX_OUTPUT);

    if (hasSerialPort) {
      serial->print("Output limits set to: ");
      serial->print(MIN_OUTPUT);
      serial->print(" - ");
      serial->println(MAX_OUTPUT);
    }
  } else if (hasSerialPort) {
    serial->println("Invalid output limits! Min must be less than max.");
  }
}

void PIDSimulator::enableSerialInput(bool enable) {
  // Only enable serial input if we have a serial port
  serialInputEnabled = enable && hasSerialPort;

  if (!hasSerialPort) {
    return;  // No serial port available, skip messages
  }

  if (serialInputEnabled) {
    serial->print("Input serial diaktifkan. Masukkan nilai Setpoint (");
    serial->print(MIN_SETPOINT);
    serial->print(" - ");
    serial->print(MAX_SETPOINT);
    serial->println("):");
  } else {
    serial->println("Input serial dinonaktifkan. Menggunakan mode setpoint manual.");
  }
}

bool PIDSimulator::isSerialInputEnabled() {
  return serialInputEnabled;
}

void PIDSimulator::setSetpointRange(double minSP, double maxSP) {
  MIN_SETPOINT = minSP;
  MAX_SETPOINT = maxSP;
}

void PIDSimulator::setOvershootParams(double gain, double initial, double damping, double maxMom) {
  MOMENTUM_GAIN = gain;
  INITIAL_MOMENTUM = initial;
  MOMENTUM_DAMPING = damping;
  MAX_MOMENTUM = maxMom;
}

void PIDSimulator::setDampingParams(double nearTarget, double atTarget) {
  NEAR_TARGET_DAMPING = nearTarget;
  AT_TARGET_DAMPING = atTarget;
}

void PIDSimulator::setNoiseAmplitude(double amplitude) {
  noiseAmplitude = amplitude;
}

void PIDSimulator::update(unsigned long interval, bool plot) {
  static unsigned long lastTime = 0;

  if (millis() - lastTime >= interval) {
    lastTime = millis();

    simulateInput();
    pid->Compute();

    if (plot) {
      sendDataToPlotter();
    }
  }
}

void PIDSimulator::checkSerialInput() {
  // Hanya periksa input serial jika mode diaktifkan dan serial port tersedia
  if (!serialInputEnabled || !hasSerialPort) {
    return;
  }

  while (serial->available()) {
    char inChar = (char)serial->read();

    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }

  if (stringComplete) {
    double newSetpoint = inputString.toFloat();

    if (newSetpoint >= MIN_SETPOINT && newSetpoint <= MAX_SETPOINT) {
      double change = newSetpoint - Setpoint;
      momentum += change * INITIAL_MOMENTUM;
      Setpoint = newSetpoint;
    } else {
      serial->print("Nilai tidak valid! Masukkan nilai antara ");
      serial->print(MIN_SETPOINT);
      serial->print(" dan ");
      serial->println(MAX_SETPOINT);
    }

    inputString = "";
    stringComplete = false;
  }
}

double PIDSimulator::getCurrentSetpoint() {
  return Setpoint;
}

double PIDSimulator::getCurrentInput() {
  return Input;
}

double PIDSimulator::getCurrentOutput() {
  return Output;
}

double PIDSimulator::getSmoothOutput() {
  double error = Setpoint - Input;
  double output;

  if (error > 0) {
    double normalizedError = error / MAX_SETPOINT;
    output = MAX_OUTPUT * (1.0 - exp(EXP_FACTOR * normalizedError));
  } else if (error < 0) {
    output = MIN_OUTPUT;
  } else {
    output = MIN_OUTPUT;
  }

  output = (SMOOTHING_FACTOR * output) + ((1 - SMOOTHING_FACTOR) * lastSmoothOutput);
  lastSmoothOutput = output;


  output = constrain(output, MIN_OUTPUT, MAX_OUTPUT);
  return output;
}

double PIDSimulator::getMinOutput() {
  return MIN_OUTPUT;
}

double PIDSimulator::getMaxOutput() {
  return MAX_OUTPUT;
}

void PIDSimulator::simulateInput() {
  // Hitung error
  double error = Setpoint - Input;

  // Normalisasi output ke range 0-1 untuk perhitungan yang lebih konsisten
  double normalizedOutput = map(Output, MIN_OUTPUT, MAX_OUTPUT, 0.0, 1.0);

  // Hitung rate perubahan berdasarkan error dan output
  double rate = 0;
  if (abs(error) > 0.01) {  // Threshold untuk menghindari osilasi kecil
    // Rate berbanding lurus dengan error dan output
    rate = (normalizedOutput * 0.1) * (error > 0 ? 1 : -1);

    // Update momentum berdasarkan error
    momentum += rate * MOMENTUM_GAIN;

    // Terapkan damping berdasarkan jarak ke target
    if (abs(error) < 1.0) {
      momentum *= NEAR_TARGET_DAMPING;
    }
  } else {
    // Jika sudah dekat target, kurangi momentum
    momentum *= AT_TARGET_DAMPING;
  }

  // Batasi momentum
  momentum = constrain(momentum, -MAX_MOMENTUM, MAX_MOMENTUM);
  momentum *= MOMENTUM_DAMPING;

  // Update nilai simulasi
  simulatedValue += rate + momentum;

  // Batasi nilai simulasi ke range setpoint
  simulatedValue = constrain(simulatedValue, MIN_SETPOINT, MAX_SETPOINT);

  // Tambahkan sedikit noise untuk realisme
  double noise = random(-100, 100) / 100.0 * noiseAmplitude;
  Input = simulatedValue + noise;
}

void PIDSimulator::sendDataToPlotter() {
  // Skip sending data if no serial port is available
  if (!hasSerialPort) {
    return;
  }

  serial->print("Lower:");
  serial->print(-MAX_SETPOINT * 1.25, 2);
  serial->print(" Upper:");
  serial->print(MAX_SETPOINT * 1.25, 2);
  serial->print(" Setpoint:");
  serial->print(Setpoint, 2);
  serial->print(" Input:");
  serial->print(Input, 2);
  serial->println();
}

double PIDSimulator::mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float PIDSimulator::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}