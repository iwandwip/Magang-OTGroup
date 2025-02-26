#include "DigitalOut.h"
#include "DistanceSensor.h"
#include "TimerOne.h"

const uint8_t LED_INDICATOR_SIZE = 4;
const float DISTANCE_RANGES[LED_INDICATOR_SIZE][2] = {
  { 1, 5 }, { 6, 10 }, { 11, 15 }, { 16, 20 }
};

DigitalOut ledLevelIndicator[LED_INDICATOR_SIZE] = {
  DigitalOut(5), DigitalOut(4), DigitalOut(6), DigitalOut(7)
};

DistanceSensor distance_sensor(2000);

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  if (distance_sensor.begin() == SENSOR_INIT_FAILED) {
    handleSensorError();
  } else {
    initializeLEDs();
    distance_sensor.startContinuous();
  }

  Timer1.initialize(1000000);  // Initial Timer1 setup (1 second)
  Timer1.attachInterrupt([]() {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  });
}

void loop() {
  sensor_err_t err_sensor_read = distance_sensor.readSingleShotDistance();

  // if (err_sensor_read != SENSOR_READ_SUCCESS) {
  //   for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
  //     ledLevelIndicator[j].toggleAsync(250);
  //   }
  //   return;
  // }

  float distance = distance_sensor.getLastDistance() / 10.0;  // toCM
  uint32_t ledDelay = getLedDelay(distance, 1, 10, 75, 375, true);
  Timer1.setPeriod(ledDelay * 1000);
  updateLEDs(distance);
  // printDistance("| mm: %4d | cm: %4d| delay: %lu", distance * 10, distance, ledDelay);
  printDistance("| cm: %4d| delay: %lu", distance, ledDelay);

  for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
    ledLevelIndicator[j].update();
  }
}

void resetArduino() {
  void (*resetFunc)(void) = 0;
  resetFunc();
}

void initializeLEDs() {
  for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
    ledLevelIndicator[j].on();
    delay(100);
    ledLevelIndicator[j].off();
    delay(100);
  }
}

void handleSensorError() {
  while (1) {
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
        ledLevelIndicator[j].toggle();
        delay(100);
      }
    }
    resetArduino();
  }
}

void updateLEDs(uint16_t distance) {
  for (int i = 0; i < LED_INDICATOR_SIZE; i++) {
    ledLevelIndicator[i].off();
  }

  for (int i = 0; i < LED_INDICATOR_SIZE; i++) {
    if (distance >= DISTANCE_RANGES[i][0] && distance <= DISTANCE_RANGES[i][1]) {
      ledLevelIndicator[i].on();
      break;
    }
  }
}

void printDistance(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[100];
  vsprintf(buffer, format, args);
  Serial.println(buffer);
  va_end(args);
}

uint32_t getLedDelay(uint16_t distance, uint16_t minDist, uint16_t maxDist, uint32_t delayMin, uint32_t delayMax, bool inverse) {
  if (minDist >= maxDist) {
    return delayMin;
  }

  if (distance < minDist) {
    distance = minDist;
  } else if (distance > maxDist) {
    distance = maxDist;
  }

  float normalizedDistance = (float)(distance - minDist) / (maxDist - minDist);
  float factor = pow(normalizedDistance, 3);

  if (inverse) {
    // Semakin jauh, delay semakin besar (delayMin ke delayMax)
    return delayMin + (uint32_t)((delayMax - delayMin) * factor);
  } else {
    // Semakin jauh, delay semakin kecil (delayMax ke delayMin)
    return delayMax - (uint32_t)((delayMax - delayMin) * factor);
  }
}
