#include "DigitalOut.h"
#include "DistanceSensor.h"

// Constants
const uint8_t LED_INDICATOR_SIZE = 4;
const uint16_t DISTANCE_RANGES[LED_INDICATOR_SIZE][2] = {
  { 10, 19 }, { 20, 29 }, { 30, 39 }, { 40, 49 }
};

// Pin Definitions
DigitalOut ledLevelIndicator[LED_INDICATOR_SIZE] = {
  DigitalOut(2), DigitalOut(3), DigitalOut(4), DigitalOut(5)
};

// Sensor Initialization
DistanceSensor distance_sensor;

// Function Prototypes
void resetArduino();
void initializeLEDs();
void handleSensorError();
void updateLEDs(uint16_t distance);
void printDistance(uint16_t distance);

void setup() {
  Serial.begin(9600);

  // Initialize sensor
  if (distance_sensor.begin() == SENSOR_INIT_FAILED) {
    handleSensorError();
  } else {
    initializeLEDs();
    distance_sensor.startContinuous();
  }
}

void loop() {
  sensor_err_t err_sensor_read = distance_sensor.readDistance();

  if (err_sensor_read == SENSOR_READ_TIMEOUT) {
    for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
      ledLevelIndicator[j].toggleAsync(250);
    }
  } else if (err_sensor_read == SENSOR_READ_SUCCESS) {
    uint16_t distance = distance_sensor.getLastDistance();
    updateLEDs(distance);
    printDistance(distance);
  }

  for (int j = 0; j < LED_INDICATOR_SIZE; j++) {
    ledLevelIndicator[j].update();
  }
}

// Function Definitions
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

void printDistance(uint16_t distance) {
  Serial.print("Distance = ");
  Serial.print(distance);
  Serial.println(" mm");
}