#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <string.h>
#include <stdio.h>
#include <AT24Cxx.h>
#include <Wire.h>
#define i2c_address 0x50
AT24Cxx eep(i2c_address, 32);
const byte pause = 2;
const byte stop = 3;
const byte start = 4;
const byte home = 5;
const byte out = 6;
const byte stopLED = 10;
const byte pauseLED = 11;
const char* param[] = { "XV", "YV", "ZV", "TV", "XL", "YL", "ZL", "TL", "XH", "YH", "ZH", "TH", "GH" };
const byte rx = 8;
const byte tx = 9;
SoftwareSerial mySerial(rx, tx);
long XV, YV, ZV, TV, XL, YL, ZL, TL, XH, YH, ZH, TH, GH;
String state = "stop";
bool automatic = false;
bool send_cmd = true;
bool portalopen = false;
int address_write = i2c_address;
String prompt = "";
byte lastStartButtonState = LOW;
byte lastHomeButtonState = LOW;
unsigned long debounceDuration = 50;
unsigned long lastTimeStartButtonStateChanged = 0;
unsigned long lastTimeHomeButtonStateChanged = 0;

void setup() {
  pinMode(out, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, OUTPUT);
  pinMode(12, OUTPUT);
  pinMode(stop, INPUT_PULLUP);
  pinMode(start, INPUT);
  pinMode(home, INPUT);
  pinMode(pause, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(stopLED, OUTPUT);
  pinMode(pauseLED, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(12, LOW);
  attachInterrupt(digitalPinToInterrupt(stop), stopbutton, FALLING);
  attachInterrupt(digitalPinToInterrupt(pause), pausebutton, FALLING);
  Serial.begin(9600);
  mySerial.begin(9600);
  XV = extract(0);
  YV = extract(1);
  ZV = extract(2);
  TV = extract(3);
  XL = extract(4);
  YL = extract(5);
  ZL = extract(6);
  TL = extract(7);
  XH = extract(8);
  YH = extract(9);
  ZH = extract(10);
  TH = extract(11);
  GH = extract(12);
}

void loop() {
  while (digitalRead(out)) {
    if (Serial.available() > 0) {
      String data = Serial.readStringUntil('\n');
      if (data == "download")
        showAllEEPROM();
      else {
        if (data == "portalopen")
          portalopen = true;
        else {
          if (data == "portalclose") {
            portalopen = false;
            address_write = i2c_address;
          } else {
            if (portalopen) {
              for (int i = 0; i < data.length(); i++)
                eep.write(address_write + i, (int)data.substring(i, i + 1)[0]);
              address_write = address_write + data.length();
            } else {
              mySerial.println(data);
              for (int i = 0; i < sizeof(param) / 2; i++) {
                int pos = data.indexOf(param[i]);
                if (pos != -1) {
                  EEPROM.write(8 * i, param[i][0]);
                  EEPROM.write(8 * i + 1, param[i][1]);
                  char beta[7];
                  data.substring(2, 8).toCharArray(beta, 7);
                  for (int j = 2; j <= 7; j++)
                    EEPROM.write(8 * i + j, beta[j - 2]);
                  showAllEEPROM();
                }
              }
            }
          }
        }
      }
    }
    if (millis() - lastTimeStartButtonStateChanged > debounceDuration) {
      byte startButtonState = digitalRead(start);
      if (startButtonState != lastStartButtonState) {
        lastTimeStartButtonStateChanged = millis();
        lastStartButtonState = startButtonState;
        if (startButtonState == LOW) {
          state = "start";
          digitalWrite(LED_BUILTIN, HIGH);
          digitalWrite(stopLED, LOW);
          digitalWrite(pauseLED, LOW);
          automatic = true;
          digitalWrite(12, LOW);
        }
      }
    }
    if (millis() - lastTimeHomeButtonStateChanged > debounceDuration) {
      byte homeButtonState = digitalRead(home);
      if (homeButtonState != lastHomeButtonState) {
        lastTimeHomeButtonStateChanged = millis();
        lastHomeButtonState = homeButtonState;
        if (homeButtonState == LOW)
          mySerial.println("X$Y$Z$T$G$");
      }
    }
    while (automatic == true && send_cmd == true) {
      int temp = eep.read(address_write);
      delay(10);
      address_write = address_write + 1;
      if (temp == 46) {  //found character "."
        SlaveSerial();
        address_write = i2c_address;
        automatic = false;
      } else {
        if (temp == 44) {  //found character ","
          SlaveSerial();
          digitalWrite(12, HIGH);
        } else {
          if (temp == 59) {  //found character ";"
            digitalWrite(12, HIGH);
            SlaveSerial();
            if (state == "pause")
              automatic = false;
            if (state == "stop") {
              automatic = false;
              address_write = i2c_address;
            }
          } else
            prompt = prompt + (char)temp;
        }
      }
    }
  }
  send_cmd = true;
}

void showAllEEPROM() {
  String eeprom_value;
  for (int i = 0; i <= 103; i++) {
    int temp = EEPROM.read(i);
    char alpha = temp;
    eeprom_value = eeprom_value + alpha;
    if (i % 8 == 7) {
      Serial.println(eeprom_value);
      mySerial.println(eeprom_value);
      eeprom_value = "";
      delay(100);
    }
  }
}

long extract(int i) {
  String result;
  for (int j = 0; j < 8; j++)
    result = result + (char)EEPROM.read(8 * i + j);
  return strtol(result.substring(2, 8).c_str(), NULL, 10);
}

String fivedigit(long input) {
  String output = "";
  input = abs(input);
  if (input < 10)
    output = "0000" + output + String(input);
  else {
    if (input < 100)
      output = "000" + output + String(input);
    else {
      if (input < 1000)
        output = "00" + output + String(input);
      else {
        if (input < 10000)
          output = "0" + output + String(input);
        else
          output = output + String(input);
      }
    }
  }
  return output;
}

/* $ berarti kalibrasi, @ berarti home, +/- berarti relative, # berarti absolute
Ada 2 kondisi. Pertama, jika bertemu string awalan X/Y/Z, ambil 1 digit setelahnya, lalu cek nilainya:
                    in case $, maka langsung diteruskan tanpa diterjemahkan
                    in case @, maka tambahkan '#' dan X+(XH)/Y+(YH)/Z+(ZH)
                    in case #, maka ambil 5 digit setelahnya (range -9999 to 99999) lalu tambahkan X+(XL)/Y+(YL)/Z+(ZL)
                    in case +/-, maka ambil 4 digit setelahnya (range -9999 to 9999) */
String conversion(String syntax) {
  String output;
  int pos = syntax.indexOf('X');
  if (pos != -1) {
    String afterindex = syntax.substring(pos + 1, pos + 2);
    if (afterindex == "$")
      output = output + "X$";
    if (afterindex == "@")
      output = output + "X#" + fivedigit(XH);
    if (afterindex == "#")
      output = output + "X#" + fivedigit(strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10) + XL);
    if (afterindex == "+" || afterindex == "-")
      output = output + "X" + syntax.substring(pos + 1, pos + 2) + fivedigit(strtol(syntax.substring(pos + 2, pos + 6).c_str(), NULL, 10));
  }
  pos = syntax.indexOf('Y');
  if (pos != -1) {
    String afterindex = syntax.substring(pos + 1, pos + 2);
    if (afterindex == "$")
      output = output + "Y$";
    if (afterindex == "@")
      output = output + "Y#" + fivedigit(YH);
    if (afterindex == "#")
      output = output + "Y#" + fivedigit(strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10) + YL);
    if (afterindex == "+" || afterindex == "-")
      output = output + "Y" + syntax.substring(pos + 1, pos + 2) + fivedigit(strtol(syntax.substring(pos + 2, pos + 6).c_str(), NULL, 10));
  }
  pos = syntax.indexOf('Z');
  if (pos != -1) {
    String afterindex = syntax.substring(pos + 1, pos + 2);
    if (afterindex == "$")
      output = output + "Z$";
    if (afterindex == "@")
      output = output + "Z#" + fivedigit(ZH);
    if (afterindex == "#")
      output = output + "Z#" + fivedigit(strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10) + ZL);
    if (afterindex == "+" || afterindex == "-")
      output = output + "Z" + syntax.substring(pos + 1, pos + 2) + fivedigit(strtol(syntax.substring(pos + 2, pos + 6).c_str(), NULL, 10));
    if (afterindex == "&")
      output = output + "Z&" + fivedigit(strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10));
  }
  /*                  Kedua, jika bertemu string awalan T dan G, ambil 1 digit setelahnya, lalu cek
                    in case $, maka langsung diteruskan tanpa diterjemahkan
                    in case @, maka tambahkan '#' dan T+(TH)/G+(GH)
                    in case #, maka ambil 2 digit setelahnya (range -9 to 99) lalu kalikan 1000
                    in case +/-, maka ambil 1 digit setelahnya (range -9 to 9) lalu kalikan 1000 */
  pos = syntax.indexOf('T');
  if (pos != -1) {
    String afterindex = syntax.substring(pos + 1, pos + 2);
    if (afterindex == "$")
      output = output + "T$";
    if (afterindex == "@") {
      if (TH < 0)
        output = output + "T#" + "-" + fivedigit(TH).substring(1, 5);
      else
        output = output + "T#" + fivedigit(TH);
    }
    if (afterindex == "#") {
      int t_capture = strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10);
      if (t_capture < 0)
        output = output + "T#" + "-" + fivedigit(t_capture).substring(1, 5);
      else
        output = output + "T#" + fivedigit(strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10));
    }
    if (afterindex == "+" || afterindex == "-")
      output = output + "T" + syntax.substring(pos + 1, pos + 2) + fivedigit(strtol(syntax.substring(pos + 2, pos + 6).c_str(), NULL, 10));
  }
  pos = syntax.indexOf('G');
  if (pos != -1) {
    String afterindex = syntax.substring(pos + 1, pos + 2);
    if (afterindex == "$")
      output = output + "G$";
    if (afterindex == "@") {
      if (GH < 0)
        output = output + "G#" + "-" + fivedigit(GH).substring(1, 5);
      else
        output = output + "G#" + fivedigit(GH);
    }
    if (afterindex == "#")
      output = output + "G#" + fivedigit(strtol(syntax.substring(pos + 2, pos + 7).c_str(), NULL, 10));
    if (afterindex == "+" || afterindex == "-")
      output = output + "G" + syntax.substring(pos + 1, pos + 2) + fivedigit(strtol(syntax.substring(pos + 2, pos + 3).c_str(), NULL, 10) * 1000);
  }
  return output;
}

void stopbutton() {
  state = "stop";
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(stopLED, HIGH);
  digitalWrite(pauseLED, LOW);
}

void pausebutton() {
  state = "pause";
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(pauseLED, HIGH);
  digitalWrite(stopLED, LOW);
}

void SlaveSerial() {
  String cmd = conversion(prompt);
  send_cmd = false;
  mySerial.println(cmd);
  if (cmd.indexOf("X#") != -1)
    digitalWrite(12, LOW);
  prompt = "";
}