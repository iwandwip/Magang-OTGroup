#include "SensorFilter.h"

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

// LerpFilter.cpp
LerpFilter::LerpFilter(float factor)
  : currentValue(0),
    targetValue(0),
    lerpFactor(factor) {}

void LerpFilter::update(float input) {
  targetValue = input;
  currentValue += lerpFactor * (targetValue - currentValue);
}

float LerpFilter::getValue() const {
  return currentValue;
}

// DynamicLerpFilter.cpp
DynamicLerpFilter::DynamicLerpFilter(
  float baseFactor,
  float maxFactor,
  float threshold)
  : currentValue(0),
    targetValue(0),
    baseLerpFactor(baseFactor),
    maxLerpFactor(maxFactor),
    proximityThreshold(threshold) {}

void DynamicLerpFilter::update(float input) {
  targetValue = input;
  float distance = abs(targetValue - currentValue);
  float dynamicLerpFactor = baseLerpFactor + (distance > proximityThreshold ? maxLerpFactor : 0);
  currentValue += dynamicLerpFactor * (targetValue - currentValue);
}

float DynamicLerpFilter::getValue() const {
  return currentValue;
}

// BiasSmoothFilter.cpp
BiasSmoothFilter::BiasSmoothFilter(
  float smooth,
  float bias)
  : smoothedValue(0),
    smoothFactor(smooth),
    biasFactor(bias) {}

void BiasSmoothFilter::update(float input) {
  float bias = abs(input - smoothedValue) > 0.05 ? biasFactor : 0;
  smoothedValue += (smoothFactor + bias) * (input - smoothedValue);
}

float BiasSmoothFilter::getValue() const {
  return smoothedValue;
}

// LowPassFilter.cpp
LowPassFilter::LowPassFilter(float factor)
  : smoothedValue(0),
    smoothFactor(factor) {}

void LowPassFilter::update(float input) {
  smoothedValue += smoothFactor * (input - smoothedValue);
}

float LowPassFilter::getValue() const {
  return smoothedValue;
}