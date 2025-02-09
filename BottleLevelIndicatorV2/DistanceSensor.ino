#include "DistanceSensor.h"

DistanceSensor::DistanceSensor(unsigned int timeoutMs)
  : m_timeoutMs(timeoutMs),
    m_lastDistance(0),
    m_inited(false) {
}

sensor_err_t DistanceSensor::begin() {
  Wire.begin();
  m_sensor.setTimeout(m_timeoutMs);
  if (!m_sensor.init()) {
    m_inited = false;
    return SENSOR_INIT_FAILED;
  }

  m_inited = true;
  return SENSOR_INIT_SUCCESS;
}

sensor_err_t DistanceSensor::readDistance() {
  if (!m_inited) {
    return SENSOR_READ_TIMEOUT;
  }
  uint16_t dist = m_sensor.readRangeContinuousMillimeters();
  if (m_sensor.timeoutOccurred()) {
    return SENSOR_READ_TIMEOUT;
  }
  m_lastDistance = dist;
  return SENSOR_READ_SUCCESS;
}

uint16_t DistanceSensor::getLastDistance() const {
  return m_lastDistance;
}

void DistanceSensor::startContinuous(uint32_t period) {
  if (!m_inited) return;
  if (period > 0) {
    m_sensor.startContinuous(period);
  } else {
    m_sensor.startContinuous();
  }
}

void DistanceSensor::stopContinuous() {
  if (!m_inited) return;
  m_sensor.stopContinuous();
}
