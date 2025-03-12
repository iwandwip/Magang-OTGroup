int clkPin = 10;
int cwPin = 11;
int enPin = 12;

void setup() {
  Serial.begin(9600);
  pinMode(clkPin, OUTPUT);
  pinMode(cwPin, OUTPUT);
  pinMode(13, OUTPUT);
}

void loop() {
  static uint32_t timerInterval = 1000;
  static uint32_t timerToggle = 0;

  if (Serial.available()) {
    timerInterval = Serial.readStringUntil('\n').toInt();
    timerInterval = timerInterval < 0 ? 0 : timerInterval;
  }

  if (millis() - timerToggle >= timerInterval) {
    timerToggle = millis();
    digitalWrite(clkPin, !digitalRead(clkPin));
    digitalWrite(13, !digitalRead(13));
  }
}
