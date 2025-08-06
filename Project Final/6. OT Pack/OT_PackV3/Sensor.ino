// ========================================
// Sensor.ino - Sensor Management Functions
// Advanced debouncing and sensor reading functions
// ========================================

// Sensor Debouncing Variables
int lastSensorState = HIGH;         // Last stable sensor state
int lastRawSensorState = HIGH;      // Last raw sensor reading
unsigned long lastDebounceTime = 0; // Last time the sensor reading changed
int sampleBuffer[DEBOUNCE_SAMPLES]; // Buffer for consistent readings
int sampleIndex = 0;                // Current index in sample buffer
unsigned long lastSampleTime = 0;   // Last time we took a sample
bool samplesInitialized = false;    // Flag to check if sample buffer is initialized

// Initialize sample buffer with current sensor reading
void initializeSampleBuffer() {
  int initialReading = digitalRead(SENSOR_PIN);
  for (int i = 0; i < DEBOUNCE_SAMPLES; i++) {
    sampleBuffer[i] = initialReading;
  }
  lastSensorState = initialReading;
  lastRawSensorState = initialReading;
  samplesInitialized = true;
}

// Advanced debouncing function with sample averaging
int getDebouncedSensorReading() {
  if (!samplesInitialized) {
    initializeSampleBuffer();
  }
  
  unsigned long currentTime = millis();
  
  // Take a new sample at specified intervals
  if (currentTime - lastSampleTime >= DEBOUNCE_SAMPLE_INTERVAL) {
    int rawReading = digitalRead(SENSOR_PIN);
    
    // Store sample in circular buffer
    sampleBuffer[sampleIndex] = rawReading;
    sampleIndex = (sampleIndex + 1) % DEBOUNCE_SAMPLES;
    lastSampleTime = currentTime;
    
    // Check if all samples are consistent
    int firstSample = sampleBuffer[0];
    bool allSame = true;
    for (int i = 1; i < DEBOUNCE_SAMPLES; i++) {
      if (sampleBuffer[i] != firstSample) {
        allSame = false;
        break;
      }
    }
    
    // If all samples are consistent and different from last state
    if (allSame && firstSample != lastRawSensorState) {
      lastDebounceTime = currentTime;
      lastRawSensorState = firstSample;
    }
    
    // If enough time has passed since last change, accept the new state
    if (allSame && (currentTime - lastDebounceTime) >= DEBOUNCE_DELAY) {
      lastSensorState = firstSample;
    }
  }
  
  return lastSensorState;
}

// Function to get current sensor reading (real or simulated)
int getSensorReading() {
  if (operationMode == MODE_NORMAL) {
    return getDebouncedSensorReading();  // Debounced real sensor reading
  } else {
    return sensorValue;  // Simulated sensor value (no debouncing needed)
  }
}