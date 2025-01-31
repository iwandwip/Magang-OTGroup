#pragma once
#include "Arduino.h"

// LerpFilter.h
#ifndef LERP_FILTER_H
#define LERP_FILTER_H

class LerpFilter {
private:
  float currentValue;
  float targetValue;
  float lerpFactor;

public:
  LerpFilter(float factor = 0.05);
  void update(float input);
  float getValue() const;
};

#endif

// DynamicLerpFilter.h
#ifndef DYNAMIC_LERP_FILTER_H
#define DYNAMIC_LERP_FILTER_H

class DynamicLerpFilter {
private:
  float currentValue;
  float targetValue;
  float baseLerpFactor;
  float maxLerpFactor;
  float proximityThreshold;

public:
  DynamicLerpFilter(
    float baseFactor = 0.02,
    float maxFactor = 0.2,
    float threshold = 0.1);
  void update(float input);
  float getValue() const;
};

#endif

// BiasSmoothFilter.h
#ifndef BIAS_SMOOTH_FILTER_H
#define BIAS_SMOOTH_FILTER_H

class BiasSmoothFilter {
private:
  float smoothedValue;
  float smoothFactor;
  float biasFactor;

public:
  BiasSmoothFilter(
    float smooth = 0.05,
    float bias = 0.01);
  void update(float input);
  float getValue() const;
};

#endif

// LowPassFilter.h
#ifndef LOW_PASS_FILTER_H
#define LOW_PASS_FILTER_H

class LowPassFilter {
private:
  float smoothedValue;
  float smoothFactor;

public:
  LowPassFilter(float factor = 0.1);
  void update(float input);
  float getValue() const;
};

#endif