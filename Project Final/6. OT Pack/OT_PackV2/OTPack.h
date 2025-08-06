#ifndef OTPACK_H
#define OTPACK_H

#include <Arduino.h>
#include <AccelStepper.h>

class OTPack {
public:
    enum State {
        WAITING_FORWARD,
        WAITING_REVERSE
    };
    
    enum MotionMode {
        BLOCKING,     // runToPosition() - blocks until motion complete
        NON_BLOCKING  // run() - returns immediately, continues in background
    };

    OTPack(byte sensorPin, byte stepPin, byte dirPin, byte enablePin);
    
    void begin();
    void update();
    
    void setMotorConfig(int stepsPerRev, int microstepping = 4);
    void setForwardProfile(float speed, float accel, int debounceMs = 150);
    void setReverseProfile(float speed, float accel, int debounceMs = 250, int settleMs = 100);
    void setPositionOffset(int offset = 2);
    void setMotionMode(MotionMode mode);
    
    State getState() const;
    MotionMode getMotionMode() const;
    bool isBusy() const;
    bool isMoving() const;  // For non-blocking mode
    void reset();

private:
    AccelStepper _stepper;
    
    byte _sensorPin;
    byte _enablePin;
    
    State _currentState;
    MotionMode _motionMode;
    bool _lastSensorState;
    unsigned long _lastStateChange;
    unsigned long _motionStartTime;
    bool _motorEnabled;
    bool _waitingForReverseSettle;
    
    int _totalSteps;
    int _positionOffset;
    
    float _forwardSpeed;
    float _forwardAccel;
    int _forwardDebounce;
    
    float _reverseSpeed;
    float _reverseAccel;
    int _reverseDebounce;
    int _reverseSettle;
    
    int _microstepping;
    
    void _executeForward();
    void _executeReverse();
    void _enableMotor();
    void _disableMotor();
    bool _readSensor();
    bool _isDebounceComplete(int debounceTime);
    void _handleNonBlockingMotion();
    bool _isReverseSettleComplete();
};

#endif