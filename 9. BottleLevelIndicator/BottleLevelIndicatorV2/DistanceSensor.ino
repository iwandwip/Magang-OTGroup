#include "DistanceSensor.h"

DistanceSensor::DistanceSensor(unsigned int timeoutMs)
  : m_timeoutMs(timeoutMs),
    m_lastDistance(0),
    m_inited(false) {
}

sensor_err_t DistanceSensor::begin() {
  Wire.begin();

  if (!lox.begin()) {
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

  // Membaca jarak tunggal dengan cara yang sama seperti mode continuous
  VL53L0X_RangingMeasurementData_t measure;
  lox.rangingTest(&measure, false);  // 'false' untuk menonaktifkan debug output

  if (measure.RangeStatus != 4) {  // Status yang valid
    m_lastDistance = measure.RangeMilliMeter;
    return SENSOR_READ_SUCCESS;
  } else {
    return SENSOR_READ_TIMEOUT;
  }
}

sensor_err_t DistanceSensor::readSingleShotDistance() {
  return readDistance();  // Fungsi yang sama untuk pembacaan tunggal
}

uint16_t DistanceSensor::getLastDistance() const {
  return m_lastDistance;
}

void DistanceSensor::startContinuous() {
  if (!m_inited) return;
  // Memulai pengukuran kontinu
  lox.startRangeContinuous();
}

void DistanceSensor::stopContinuous() {
  if (!m_inited) return;
  // Menghentikan pengukuran kontinu (untuk aplikasi yang membutuhkan penghentian)
  lox.stopRangeContinuous();
}

bool DistanceSensor::isRangeComplete() {
  // Memeriksa apakah pengukuran kontinu telah selesai
  return lox.isRangeComplete();
}
