#ifndef DISTANCE_SENSOR_H
#define DISTANCE_SENSOR_H

#include "Arduino.h"
#include "Wire.h"
#include "Adafruit_VL53L0X.h"

typedef enum {
  SENSOR_INIT_SUCCESS,
  SENSOR_INIT_FAILED,
  SENSOR_READ_SUCCESS,
  SENSOR_READ_TIMEOUT,
} sensor_err_t;

class DistanceSensor {
public:
  DistanceSensor(unsigned int timeoutMs = 500);
  sensor_err_t begin();
  sensor_err_t readDistance();
  sensor_err_t readSingleShotDistance();
  uint16_t getLastDistance() const;
  void startContinuous();
  void stopContinuous();
  bool isRangeComplete();  // Untuk memeriksa apakah pembacaan selesai dalam mode continuous

private:
  Adafruit_VL53L0X lox;
  unsigned int m_timeoutMs;
  uint16_t m_lastDistance;
  bool m_inited;
};

#endif
