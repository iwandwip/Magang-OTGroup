#include <AltSoftSerial.h>
#include <AccelStepper.h>
#include <EEPROM.h>
const byte en = 12;
const byte cw = 11;
const byte clk = 10;
const byte sensor = 6;
const double accel_speed_ratio = 0.6;
AccelStepper stepper(AccelStepper::DRIVER, clk, cw);
double maxspeed, accel;
long offsetLayer = 0;
long offsetHome = 0;
long absolute;
const byte rx = 8;
const byte tx = 9;
AltSoftSerial mySerial;
int Trajectory;
double track_left;
const char slave = 'Z';

void setup() {
  if (slave == 'X')
    track_left = 0.1;
  if (slave == 'Y')
    track_left = 0.1;
  if (slave == 'Z')
    track_left = 0.1;
  if (slave == 'T')
    track_left = 0;
  if (slave == 'G')
    track_left = 0.1;
  pinMode(en, OUTPUT);
  pinMode(cw, OUTPUT);
  pinMode(clk, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(8, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(en, LOW);
  digitalWrite(cw, LOW);
  Serial.begin(9600);
  mySerial.begin(9600);
  absolute = 0;
  if (slave == 'G')
    maxspeed = 80000;
  else
    maxspeed = extract();
  stepper.setMaxSpeed(maxspeed);
  accel = maxspeed * accel_speed_ratio;
  stepper.setAcceleration(accel);
  stepper.setCurrentPosition(0);
}

void loop() {
ex:
  if (mySerial.available() > 0 && digitalRead(LED_BUILTIN)) {
    String data = mySerial.readStringUntil('\n');
    Serial.println(data);
    int pos = data.indexOf(slave);
    if (pos != -1) {
      digitalWrite(LED_BUILTIN, LOW);
      String afterindex = data.substring(pos + 1, pos + 2);
      if (afterindex == "V") {
        EEPROM.update(0, slave);
        EEPROM.update(1, 'V');
        char beta[7];
        data.substring(2, 8).toCharArray(beta, 7);
        for (int i = 2; i <= 7; i++)
          EEPROM.update(i, beta[i - 2]);
        maxspeed = extract();
        stepper.setMaxSpeed(maxspeed);
        accel = maxspeed * accel_speed_ratio;
        stepper.setAcceleration(accel);
        digitalWrite(LED_BUILTIN, HIGH);
      }
      if (afterindex == "A" || afterindex == "L" || afterindex == "H")
        digitalWrite(LED_BUILTIN, HIGH);
      if (afterindex == "$") {
        calibration();
        digitalWrite(LED_BUILTIN, HIGH);
      }
      if (afterindex == "+" || afterindex == "-" || afterindex == "#" || afterindex == "&") {
        if (slave == 'X' || slave == 'Y' || slave == 'T')
          delay(500);
        if (afterindex == "#" || afterindex == "&") {
          absolute = strtol(data.substring(pos + 2, pos + 7).c_str(), NULL, 10);
          if (slave == 'Z' && afterindex == "&") {
            stepper.moveTo(0);
            stepper.runToPosition();
            stepper.moveTo(absolute);
            delay(1000);
            stepper.runToPosition();
            digitalWrite(LED_BUILTIN, HIGH);
            goto ex;
          } else
            stepper.moveTo(absolute);
        } else
          stepper.move(strtol(data.substring(pos + 1, pos + 7).c_str(), NULL, 10));
        Trajectory = stepper.distanceToGo();
        //        if(slave == 'T') {
        //          stepper.runToPosition();
        //          digitalWrite(LED_BUILTIN,HIGH);
        //        }
      }
    }
  }
  //  if(slave == 'T') {
  //    int reading_int = myEnc.read();
  //    if(abs(absolute - reading_int)>50) {
  //      stepper.move(absolute - reading_int);
  //      stepper.runToPosition();
  //    }
  //  }
  else {
    if (stepper.distanceToGo() != 0)
      stepper.run();
    if (abs(stepper.distanceToGo()) <= abs(Trajectory) * track_left)
      digitalWrite(LED_BUILTIN, HIGH);
  }
}

long extract() {
  String result;
  for (int i = 0; i < 8; i++)
    result = result + (char)EEPROM.read(i);
  return strtol(result.substring(2, 8).c_str(), NULL, 10);
}

void calibration() {
  long distance = 0;
  if (slave == 'T') {
    stepper.setMaxSpeed(50);
    stepper.setAcceleration(25);
  } else {
    stepper.setMaxSpeed(250);
    stepper.setAcceleration(125);
  }
  int count = 20000;
  if (digitalRead(sensor) == HIGH) {  //inside Home
    stepper.move(count);
    do {
      stepper.run();
    } while (digitalRead(sensor) == HIGH);
  } else {  //outside Home
    stepper.move(-count);
    do {
      stepper.run();
    } while (digitalRead(sensor) == LOW);
  }
  stepper.stop();
  distance = stepper.distanceToGo();
  stepper.runToPosition();
  stepper.move(-distance);
  stepper.runToPosition();
  stepper.setCurrentPosition(0);
  absolute = 0;
  stepper.setMaxSpeed(maxspeed);
  stepper.setAcceleration(accel);
}