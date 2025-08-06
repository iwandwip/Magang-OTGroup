#include "OTPack.h"

OTPack::OTPack(byte sensorPin, byte stepPin, byte dirPin, byte enablePin)
    : _stepper(AccelStepper::DRIVER, stepPin, dirPin),
      _sensorPin(sensorPin),
      _enablePin(enablePin),
      _currentState(WAITING_FORWARD),
      _motionMode(BLOCKING),
      _lastSensorState(false),
      _lastStateChange(0),
      _motionStartTime(0),
      _motorEnabled(false),
      _waitingForReverseSettle(false),
      _totalSteps(232),
      _positionOffset(2),
      _forwardSpeed(1200.0),
      _forwardAccel(600.0),
      _forwardDebounce(150),
      _reverseSpeed(3000.0),
      _reverseAccel(1900.0),
      _reverseDebounce(250),
      _reverseSettle(100),
      _microstepping(4) {
}

void OTPack::begin() {
    pinMode(_sensorPin, INPUT_PULLUP);
    pinMode(_enablePin, OUTPUT);
    _disableMotor();
    
    _stepper.setMaxSpeed(_forwardSpeed * _microstepping);
    _stepper.setAcceleration(_forwardAccel * _microstepping);
    
    _lastSensorState = _readSensor();
    _lastStateChange = millis();
    
    Serial.println("OTPack initialized");
}

void OTPack::update() {
    bool currentSensor = _readSensor();
    
    // Handle non-blocking motion updates
    if (_motionMode == NON_BLOCKING) {
        _handleNonBlockingMotion();
    }
    
    // Handle reverse settle delay in non-blocking mode
    if (_waitingForReverseSettle && _isReverseSettleComplete()) {
        _disableMotor();
        _waitingForReverseSettle = false;
        _currentState = WAITING_FORWARD;
        _lastStateChange = millis();
        Serial.println("Reverse complete");
    }
    
    // State machine logic - only trigger new motions if not currently moving
    if (!isMoving() && !_waitingForReverseSettle) {
        switch (_currentState) {
            case WAITING_FORWARD:
                if (currentSensor && !_lastSensorState && _isDebounceComplete(_forwardDebounce)) {
                    _executeForward();
                    _currentState = WAITING_REVERSE;
                    _lastStateChange = millis();
                }
                break;
                
            case WAITING_REVERSE:
                if (!currentSensor && _lastSensorState && _isDebounceComplete(_reverseDebounce)) {
                    _executeReverse();
                    if (_motionMode == BLOCKING) {
                        _currentState = WAITING_FORWARD;
                    } else {
                        // In non-blocking mode, state will change after motion + settle completes
                    }
                    _lastStateChange = millis();
                }
                break;
        }
    }
    
    if (currentSensor != _lastSensorState) {
        _lastSensorState = currentSensor;
        _lastStateChange = millis();
    }
}

void OTPack::setMotorConfig(int stepsPerRev, int microstepping) {
    _microstepping = microstepping;
    _totalSteps = stepsPerRev * microstepping;
}

void OTPack::setForwardProfile(float speed, float accel, int debounceMs) {
    _forwardSpeed = speed;
    _forwardAccel = accel;
    _forwardDebounce = debounceMs;
}

void OTPack::setReverseProfile(float speed, float accel, int debounceMs, int settleMs) {
    _reverseSpeed = speed;
    _reverseAccel = accel;
    _reverseDebounce = debounceMs;
    _reverseSettle = settleMs;
}

void OTPack::setPositionOffset(int offset) {
    _positionOffset = offset;
}

void OTPack::setMotionMode(MotionMode mode) {
    _motionMode = mode;
}

OTPack::State OTPack::getState() const {
    return _currentState;
}

OTPack::MotionMode OTPack::getMotionMode() const {
    return _motionMode;
}

bool OTPack::isBusy() const {
    return isMoving() || _waitingForReverseSettle;
}

bool OTPack::isMoving() const {
    return _stepper.distanceToGo() != 0;
}

void OTPack::reset() {
    _stepper.stop();
    _stepper.setCurrentPosition(0);
    _currentState = WAITING_FORWARD;
    _waitingForReverseSettle = false;
    _disableMotor();
    _lastStateChange = millis();
}

void OTPack::_executeForward() {
    Serial.println("Forward motion");
    
    _stepper.setMaxSpeed(_forwardSpeed * _microstepping);
    _stepper.setAcceleration(_forwardAccel * _microstepping);
    
    _enableMotor();
    _stepper.move(_totalSteps);
    _motionStartTime = millis();
    
    if (_motionMode == BLOCKING) {
        _stepper.runToPosition();
        Serial.println("Forward complete");
    }
}

void OTPack::_executeReverse() {
    Serial.println("Reverse motion");
    
    _stepper.setMaxSpeed(_reverseSpeed * _microstepping);
    _stepper.setAcceleration(_reverseAccel * _microstepping);
    
    _stepper.move(-(_totalSteps - _positionOffset));
    _motionStartTime = millis();
    
    if (_motionMode == BLOCKING) {
        _stepper.runToPosition();
        delay(_reverseSettle);
        _disableMotor();
        Serial.println("Reverse complete");
    }
}

void OTPack::_enableMotor() {
    digitalWrite(_enablePin, HIGH);
    _motorEnabled = true;
}

void OTPack::_disableMotor() {
    digitalWrite(_enablePin, LOW);
    _motorEnabled = false;
}

bool OTPack::_readSensor() {
    return digitalRead(_sensorPin) == HIGH;
}

bool OTPack::_isDebounceComplete(int debounceTime) {
    return (millis() - _lastStateChange) >= debounceTime;
}

void OTPack::_handleNonBlockingMotion() {
    if (_stepper.distanceToGo() != 0) {
        _stepper.run();
    } else if (_motorEnabled && _stepper.distanceToGo() == 0) {
        // Motion just completed
        if (_currentState == WAITING_REVERSE) {
            // Forward motion completed
            Serial.println("Forward complete");
        } else if (_currentState == WAITING_FORWARD) {
            // Reverse motion completed, start settle timer
            _waitingForReverseSettle = true;
            _motionStartTime = millis();
        }
    }
}

bool OTPack::_isReverseSettleComplete() {
    return (millis() - _motionStartTime) >= _reverseSettle;
}