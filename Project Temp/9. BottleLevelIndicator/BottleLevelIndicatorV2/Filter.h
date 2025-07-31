#pragma once
#include "Arduino.h"

class MovingAverageFilter {
private:
  int _windowSize;
  int _currentIndex;
  float* _values;
  float _runningSum;
public:
  explicit MovingAverageFilter(int windowSize);
  ~MovingAverageFilter();
  void addMeasurement(float value);
  float getFilteredValue() const;
  void clear();
};