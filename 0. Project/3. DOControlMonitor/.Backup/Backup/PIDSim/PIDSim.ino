#include <PID_v1.h>

#define PIN_INPUT 0
#define PIN_OUTPUT 3

const double MIN_SETPOINT = 0.0;
const double MAX_SETPOINT = 100.0;

// Parameter untuk mengatur overshoot
const double MOMENTUM_GAIN = 0.05;       // Mengontrol seberapa besar penambahan momentum
const double INITIAL_MOMENTUM = 0.08;    // Mengontrol momentum awal saat setpoint berubah
const double MOMENTUM_DAMPING = 0.85;    // Mengontrol redaman (0.7-0.9: cepat, 0.9-0.98: lambat)
const double MAX_MOMENTUM = 20.0;        // Batas maksimum momentum untuk membatasi overshoot
const double NEAR_TARGET_DAMPING = 0.9;  // Redaman tambahan saat mendekati target
const double AT_TARGET_DAMPING = 0.7;    // Redaman saat sangat dekat dengan target

double Setpoint, Input, Output;
double Kp = 2, Ki = 5, Kd = 1;

PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

unsigned long lastTime = 0;
double simulatedValue = 0;
double noiseAmplitude = 0.5;
double momentum = 0;

String inputString = "";
boolean stringComplete = false;

void setup() {
  Serial.begin(9600);

  Setpoint = 25.5;
  Input = 0.0;

  myPID.SetOutputLimits(0, 255);
  myPID.SetMode(AUTOMATIC);

  Serial.print("Masukkan nilai Setpoint (");
  Serial.print(MIN_SETPOINT);
  Serial.print(" - ");
  Serial.print(MAX_SETPOINT);
  Serial.println("):");
  delay(100);
}

void loop() {
  checkSerialInput();

  if (millis() - lastTime >= 50) {
    lastTime = millis();

    simulateInput();
    myPID.Compute();

    Serial.print("Lower:");
    Serial.print(-MAX_SETPOINT * 1.25, 2);
    Serial.print(" Upper:");
    Serial.print(MAX_SETPOINT * 1.25, 2);

    Serial.print(" Setpoint:");
    Serial.print(Setpoint, 2);
    Serial.print(" Input:");
    Serial.print(Input, 2);
    Serial.println();
  }
}

void simulateInput() {
  double rate;
  double distance = Setpoint - Input;

  // Menghitung rate berdasarkan Output dan jarak ke Setpoint
  if (abs(distance) > 0.1) {
    if (distance > 0) {
      // Saat naik, gunakan rate yang lebih tinggi untuk menciptakan momentum
      rate = (Output / 255.0) * 1.2;
      // Tambahkan momentum untuk overshoot
      momentum += rate * MOMENTUM_GAIN;
    } else {
      // Saat turun, gunakan rate konstan
      rate = 1.0;
      // Tambahkan momentum negatif
      momentum -= rate * MOMENTUM_GAIN;
    }

    // Terapkan momentum untuk menggerakkan nilai
    simulatedValue += rate * (distance > 0 ? 1 : -1) + momentum;

    // Redaman tambahan ketika mendekati setpoint
    if (abs(distance) < 10) {
      momentum *= NEAR_TARGET_DAMPING;
    }
  } else {
    // Dekat setpoint, hanya terapkan momentum
    simulatedValue += momentum;
    // Redaman ekstra saat sangat dekat dengan setpoint
    momentum *= AT_TARGET_DAMPING;
  }

  // Kurangi momentum seiring waktu (redaman)
  momentum *= MOMENTUM_DAMPING;

  // Batasi momentum maksimum untuk mencegah overshoot berlebihan
  if (momentum > MAX_MOMENTUM) momentum = MAX_MOMENTUM;
  if (momentum < -MAX_MOMENTUM) momentum = -MAX_MOMENTUM;

  // Tambahkan noise
  double noise = random(-100, 100) / 100.0 * noiseAmplitude;
  Input = simulatedValue + noise;
}

void checkSerialInput() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();

    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }

  if (stringComplete) {
    double newSetpoint = inputString.toFloat();

    if (newSetpoint >= MIN_SETPOINT && newSetpoint <= MAX_SETPOINT) {
      // Tambahkan momentum awal saat setpoint berubah
      double change = newSetpoint - Setpoint;
      momentum += change * INITIAL_MOMENTUM;
      Setpoint = newSetpoint;
    } else {
      Serial.print("Nilai tidak valid! Masukkan nilai antara ");
      Serial.print(MIN_SETPOINT);
      Serial.print(" dan ");
      Serial.println(MAX_SETPOINT);
    }

    inputString = "";
    stringComplete = false;
  }
}