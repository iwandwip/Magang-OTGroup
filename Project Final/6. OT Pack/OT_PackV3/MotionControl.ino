// ========================================
// MotionControl.ino - Advanced Motion Control
// Smooth motion functions with jerk control and resonance avoidance
// ========================================

// Motion Smoothness Variables
int jerkReductionSteps = DEFAULT_JERK_REDUCTION_STEPS;
int enableRampDelay = DEFAULT_ENABLE_RAMP_DELAY;
int disableRampDelay = DEFAULT_DISABLE_RAMP_DELAY;
float speedRampFactor = DEFAULT_SPEED_RAMP_FACTOR;
int motionSettleDelay = DEFAULT_MOTION_SETTLE_DELAY;

// Function to gradually enable motor with ramping
void gradualEnableMotor() {
  // Gradual enable to reduce electrical shock
  for (int i = 0; i < 5; i++) {
    digitalWrite(ENABLE_PIN, HIGH);
    delayMicroseconds(100);
    digitalWrite(ENABLE_PIN, LOW);
    delay(enableRampDelay / 5);
  }
  digitalWrite(ENABLE_PIN, HIGH);
  delay(motionSettleDelay);
}

// Function to gradually disable motor with ramping
void gradualDisableMotor() {
  delay(motionSettleDelay);
  // Gradual disable to reduce mechanical shock
  for (int i = 0; i < 3; i++) {
    digitalWrite(ENABLE_PIN, LOW);
    delay(disableRampDelay / 3);
    digitalWrite(ENABLE_PIN, HIGH);
    delayMicroseconds(50);
  }
  digitalWrite(ENABLE_PIN, LOW);
}

// Function to check and adjust speed to avoid resonance
float avoidResonance(float requestedSpeed) {
  if (requestedSpeed >= RESONANCE_AVOID_MIN_SPEED && 
      requestedSpeed <= RESONANCE_AVOID_MAX_SPEED) {
    // Speed is in resonance zone, adjust to safe speed
    if (requestedSpeed < (RESONANCE_AVOID_MIN_SPEED + RESONANCE_AVOID_MAX_SPEED) / 2) {
      return RESONANCE_AVOID_MIN_SPEED - 50.0;  // Go below resonance
    } else {
      return RESONANCE_SAFE_SPEED;  // Go above resonance
    }
  }
  return requestedSpeed;  // Speed is safe, no adjustment needed
}

// Advanced S-curve motion with jerk control
void performSmoothMotion(long steps, float maxSpeed, float acceleration, bool isExtend) {
  // Apply resonance avoidance
  float safeSpeed = avoidResonance(maxSpeed);
  
  // Calculate S-curve parameters
  float jerkSteps = min(jerkReductionSteps, abs(steps) / 4);
  float rampSpeed = safeSpeed * speedRampFactor;
  
  // Phase 1: Jerk-limited acceleration start
  stepper.setMaxSpeed(rampSpeed * MICROSTEPPING_RESOLUTION);
  stepper.setAcceleration(acceleration * 0.3 * MICROSTEPPING_RESOLUTION);
  stepper.move(jerkSteps * (steps > 0 ? 1 : -1));
  stepper.runToPosition();
  
  // Phase 2: Normal acceleration to max speed
  stepper.setMaxSpeed(safeSpeed * MICROSTEPPING_RESOLUTION);
  stepper.setAcceleration(acceleration * MICROSTEPPING_RESOLUTION);
  stepper.move((abs(steps) - 2 * jerkSteps) * (steps > 0 ? 1 : -1));
  stepper.runToPosition();
  
  // Phase 3: Jerk-limited deceleration end
  stepper.setMaxSpeed(rampSpeed * MICROSTEPPING_RESOLUTION);
  stepper.setAcceleration(acceleration * 0.3 * MICROSTEPPING_RESOLUTION);
  stepper.move(jerkSteps * (steps > 0 ? 1 : -1));
  stepper.runToPosition();
  
  // Settling delay for mechanical stability
  delay(motionSettleDelay);
}

// Enhanced extend motion with smoothness
void performSmoothExtendMotion() {
  Serial.println(F("Starting smooth extend motion..."));
  
  // Gradual motor enable
  gradualEnableMotor();
  
  // Pre-motion delay
  delay(extendDelayBefore);
  
  // Smooth S-curve motion
  performSmoothMotion(stepsPerRevolution, extendMaxSpeed, extendAcceleration, true);
  
  isExtended = true;
  Serial.println(F("Smooth extend motion completed."));
}

// Enhanced retract motion with smoothness
void performSmoothRetractMotion() {
  Serial.println(F("Starting smooth retract motion..."));
  
  // Pre-motion delay
  delay(retractDelayBefore);
  
  // Smooth S-curve motion (with step adjustment)
  long retractSteps = -stepsPerRevolution + retractStepAdjustment;
  performSmoothMotion(retractSteps, retractMaxSpeed, retractAcceleration, false);
  
  // Post-motion delay
  delay(retractDelayAfter);
  
  // Gradual motor disable
  gradualDisableMotor();
  
  isExtended = false;
  Serial.println(F("Smooth retract motion completed."));
}