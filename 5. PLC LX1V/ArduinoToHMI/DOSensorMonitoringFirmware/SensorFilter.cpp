#include "SensorFilter.h"

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