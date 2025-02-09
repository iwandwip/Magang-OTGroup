#include "DigitalOut.h"
#include "DistanceSensor.h"

const uint8_t LED_INDICATOR_SIZE = 4;
DigitalOut ledLevelIndicator[LED_INDICATOR_SIZE] = {
  DigitalOut(2),
  DigitalOut(3),
  DigitalOut(4),
  DigitalOut(5)
};

DistanceSensor distance_sensor;

void (*resetArduino)(void) = 0;

void setup() {
  Serial.begin(9600);
  sensor_err_t err_sensor_init = distance_sensor.begin();
  if (err_sensor_init == SENSOR_INIT_FAILED) {
    while (1) {
      for (int i = 0; i < 10; i++) {
        for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
          ledLevelIndicator[j].toggle();
          delay(100);
        }
      }
      resetArduino();
    }
  } else {
    for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
      ledLevelIndicator[j].on();
      delay(100);
      ledLevelIndicator[j].off();
      delay(100);
    }
  }
  distance_sensor.startContinuous();
}

void loop() {
  sensor_err_t err_sensor_read = distance_sensor.readDistance();
  if (err_sensor_read == SENSOR_READ_TIMEOUT) {
    for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
      ledLevelIndicator[j].toggleAsync(250);
    }
  } else if (err_sensor_read == SENSOR_READ_SUCCESS) {
    uint16_t dist = distance_sensor.getLastDistance();

    for (int i = 0; i < LED_INDICATOR_SIZE; i++) {
      ledLevelIndicator[i].off();
    }

    if (dist >= 10 && dist < 20) ledLevelIndicator[0].on();
    else if (dist >= 20 && dist < 30) ledLevelIndicator[1].on();
    else if (dist >= 30 && dist < 40) ledLevelIndicator[2].on();
    else if (dist >= 40 && dist < 50) ledLevelIndicator[3].on();

    Serial.print("Distance = ");
    Serial.print(dist);
    Serial.println(" mm");
  }

  for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
    ledLevelIndicator[j].update();
  }
}