#include "EEPROM.h"

const byte radar = 2;
const int ac220 = A0;
const byte in6 = 9;   //filling tank pump
const byte in5 = 11;  //dosing pump
const byte in4 = 8;   //transfer pump
int delay1, delay2;
long timenow;
bool acpresent;
String state;

void setup() {
  Serial.begin(9600);
  pinMode(radar, INPUT_PULLUP);
  pinMode(in6, OUTPUT);
  pinMode(in5, OUTPUT);
  pinMode(in4, OUTPUT);
  digitalWrite(in4, LOW);
  digitalWrite(in5, LOW);
  digitalWrite(in6, LOW);
  delay(1000);
  delay1 = EEPROM.read(0);
  delay2 = EEPROM.read(1);
  state = "waiting";
}
void loop() {
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    int commaIndex = data.indexOf(',');
    if (commaIndex > 0) {
      String firstValue = data.substring(0, commaIndex);
      String secondValue = data.substring(commaIndex + 1);
      delay1 = firstValue.toInt();
      EEPROM.write(0, delay1);
      delay2 = secondValue.toInt();
      EEPROM.write(1, delay2);
    } else {
      if (data == "download") {
        Serial.print(EEPROM.read(0));
        Serial.print(",");
        Serial.println(EEPROM.read(1));
      } else
        Serial.println("ERROR");
    }
  }

  if (state == "waiting") {
    float voltage;
    float sumvoltage = 0;
    for (int i = 0; i < 500; i++) {
      voltage = abs(analogRead(ac220) - 512);
      sumvoltage = sumvoltage + voltage * voltage;
    }
    sumvoltage = sumvoltage / 500;
    if (sumvoltage > 500) {
      digitalWrite(in6, HIGH);  //start fill tank
      digitalWrite(in5, HIGH);  //start dosing pac
      state = "filling";
    }
  }
  if (digitalRead(radar) == LOW && state == "filling") {  //Tank full
    bool validasi = true;
    for (int i = 0; i < 200; i++) {
      if (digitalRead(radar) == HIGH) {
        validasi = false;
        break;
      }
    }
    if (validasi == true) {
      digitalWrite(in6, LOW);   //stop fill tank
      digitalWrite(in4, HIGH);  //start transfer 400
      timenow = millis();
      state = "transfer";
    }
  }
  if (millis() - timenow > 200000 && state == "transfer") {
    digitalWrite(in4, LOW);  //stop transfer 400
    digitalWrite(in5, LOW);  //stop dosing pac
    state = "waiting";
  }
}