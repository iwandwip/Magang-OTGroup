#include "Filter.h"

MovingAverageFilter::MovingAverageFilter(int windowSize) {
  _windowSize = windowSize;
  _currentIndex = 0;
  _values = new float[windowSize];
  _runningSum = 0.0;
  for (int i = 0; i < windowSize; ++i) {
    _values[i] = 0.0;
  }
}

MovingAverageFilter::~MovingAverageFilter() {
  delete[] _values;
}

void MovingAverageFilter::addMeasurement(float value) {
  _runningSum -= _values[_currentIndex];
  _runningSum += value;
  _values[_currentIndex] = value;
  _currentIndex = (_currentIndex + 1) % _windowSize;
}

float MovingAverageFilter::getFilteredValue() const {
  return _runningSum / _windowSize;
}

void MovingAverageFilter::clear() {
  _runningSum = 0.0;
  for (int i = 0; i < _windowSize; ++i) {
    _values[i] = 0.0;
  }
}