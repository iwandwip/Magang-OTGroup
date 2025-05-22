/*
 * P100S AC Servo Drive Controller (Minimal)
 * 
 * Koneksi:
 * - Arduino Pin 2 -> CTRG (DI1) - Trigger
 * - Arduino Pin 3 -> POS0 (DI2) - Bit 0 pemilihan posisi
 * - Arduino Pin 4 -> POS1 (DI3) - Bit 1 pemilihan posisi
 * - Arduino Pin 5 -> POS2 (DI4) - Bit 2 pemilihan posisi
 * - Arduino Pin 6 <- ZSP  (DO4) - Zero Speed Signal
 * 
 * Parameter servo drive:
 * - PA4=0, PA14=3, P3-0=28, P3-1=29, P3-2=30, P3-3=31, P3-23=4, P4-0=0, P4-1=0
 */

// Definisi pin
const int CTRG_PIN = 12;
const int POS0_PIN = 11;
const int POS1_PIN = 10;
const int POS2_PIN = A5;
const int ZSP_PIN = 6;
const int BRAKE_PIN = 7;

// Konstanta
const int TRIGGER_PULSE = 100;  // ms - durasi pulsa trigger
const int MAX_TIMEOUT = 10000;  // ms - timeout menunggu motor berhenti

void setup() {
  pinMode(CTRG_PIN, OUTPUT);
  pinMode(POS0_PIN, OUTPUT);
  pinMode(POS1_PIN, OUTPUT);
  pinMode(POS2_PIN, OUTPUT);
  pinMode(BRAKE_PIN, OUTPUT);
  pinMode(ZSP_PIN, INPUT_PULLUP);

  digitalWrite(CTRG_PIN, LOW);
  digitalWrite(POS0_PIN, LOW);
  digitalWrite(POS1_PIN, LOW);
  digitalWrite(POS2_PIN, LOW);
  digitalWrite(BRAKE_PIN, LOW);  // relay

  Serial.begin(9600);
  Serial.println(F("\nP100S Servo Controller"));
  showMenu();
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == '\n' || command == '\r') {
      return;
    }
    processCommand(command);
  }
}

void showMenu() {
  Serial.println(F("\n--------------------------"));
  Serial.println(F("1-8: Pilih posisi"));
  Serial.println(F("S: Status servo"));
  Serial.println(F("H: Bantuan"));
  Serial.println(F("--------------------------"));
}

void processCommand(char command) {
  switch (command) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
      moveToPosition(command - '0');
      break;
    case 's':
    case 'S':
      checkStatus();
      showMenu();
      break;
    case 'h':
    case 'H':
    case '?':
      showHelp();
      showMenu();
      break;
    default:
      Serial.println(F("Perintah tidak dikenal. Tekan 'H' untuk bantuan."));
      showMenu();
      break;
  }
}

void moveToPosition(int position) {
  if (position < 1 || position > 8) {
    Serial.println(F("Posisi tidak valid. Harus antara 1-8."));
    return;
  }

  Serial.print(F("\nBergerak ke posisi "));
  Serial.println(position);

  if (!waitForMotorStop()) {
    Serial.println(F("Motor tidak berhenti dalam waktu yang ditentukan."));
    showMenu();
    return;
  }

  // Konversi posisi (1-8) ke kode biner 3-bit (000-111)
  int posValue = position - 1;

  digitalWrite(POS0_PIN, posValue & 0x01);
  digitalWrite(POS1_PIN, (posValue >> 1) & 0x01);
  digitalWrite(POS2_PIN, (posValue >> 2) & 0x01);

  Serial.print(F("POS2:"));
  Serial.print(digitalRead(POS2_PIN));
  Serial.print(F(", POS1:"));
  Serial.print(digitalRead(POS1_PIN));
  Serial.print(F(", POS0:"));
  Serial.println(digitalRead(POS0_PIN));

  delay(50);

  Serial.println(F("Mengirim trigger..."));

  // for (int i = 0; i < 4; i++) {
  //   digitalWrite(CTRG_PIN, HIGH);
  //   delay(TRIGGER_PULSE);
  //   digitalWrite(CTRG_PIN, LOW);
  //   delay(TRIGGER_PULSE / 2);
  // }

  // digitalWrite(CTRG_PIN, HIGH);
  // delay(3000);
  // digitalWrite(CTRG_PIN, LOW);

  digitalWrite(CTRG_PIN, HIGH);
  delay(150);
  digitalWrite(CTRG_PIN, LOW);

  Serial.println(F("Motor bergerak..."));

  if (waitForMotorStop()) {
    Serial.println(F("Pergerakan selesai!"));
  } else {
    Serial.println(F("Timeout! Motor mungkin masih bergerak."));
  }

  showMenu();
}

bool waitForMotorStop() {
  Serial.println(F("Menunggu motor berhenti..."));

  unsigned long startTime = millis();
  delay(100);

  if (digitalRead(ZSP_PIN) == HIGH) {
    Serial.println(F("Motor sudah berhenti"));
    return true;
  }

  while (digitalRead(ZSP_PIN) != HIGH) {
    delay(100);

    if ((millis() - startTime) % 1000 < 100) {
      Serial.print(F("."));
    }

    if (millis() - startTime > MAX_TIMEOUT) {
      Serial.println(F("\nTimeout menunggu motor berhenti!"));
      return false;
    }

    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == 'x' || c == 'X') {
        Serial.println(F("\nDibatalkan oleh pengguna!"));
        return false;
      }
    }
  }

  Serial.println(F("\nMotor telah berhenti"));
  return true;
}

void checkStatus() {
  Serial.println(F("\n--- STATUS SERVO ---"));

  bool isZeroSpeed = digitalRead(ZSP_PIN) == HIGH;
  Serial.print(F("Kecepatan Nol (ZSP): "));
  Serial.println(isZeroSpeed ? F("YA (motor diam)") : F("TIDAK (motor bergerak)"));

  Serial.println(F("\nStatus Pin:"));
  Serial.print(F("CTRG: "));
  Serial.println(digitalRead(CTRG_PIN) == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F("POS0: "));
  Serial.println(digitalRead(POS0_PIN) == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F("POS1: "));
  Serial.println(digitalRead(POS1_PIN) == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F("POS2: "));
  Serial.println(digitalRead(POS2_PIN) == HIGH ? F("HIGH") : F("LOW"));
  Serial.print(F("ZSP : "));
  Serial.println(digitalRead(ZSP_PIN) == HIGH ? F("HIGH") : F("LOW"));
}

void showHelp() {
  Serial.println(F("\n--- BANTUAN ---"));
  Serial.println(F("1-8: Bergerak ke posisi 1-8"));
  Serial.println(F("S: Status servo"));
  Serial.println(F("X: Batalkan (saat motor bergerak)"));

  Serial.println(F("\nParameter P100S: PA4=0, PA14=3, P3-0=28, P3-1=29, P3-2=30, P3-3=31, P3-23=4"));

  Serial.println(F("\nPin: 2->CTRG, 3->POS0, 4->POS1, 5->POS2, 6<-ZSP"));
}