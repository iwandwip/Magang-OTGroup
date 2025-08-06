//
// AccelInterruptStepper.h - Enhanced stepper library combining AccelStepper flexibility with GRBL interrupt system
//
// Copyright (C) 2024 AccelInterruptStepper Project
// Based on AccelStepper by Mike McCauley and GRBL by Sungeun K. Jeon
//
// This library combines the best of both worlds:
// - AccelStepper's rich API and David Austin's speed algorithm
// - GRBL's interrupt-based multi-stepper Bresenham algorithm
//
// Key Features:
// - Timer1 interrupt-driven execution (no delays from main program)
// - Up to 4 steppers coordinated with Bresenham algorithm
// - Maintains AccelStepper API compatibility where possible
// - AMASS (Adaptive Multi-Axis Step Smoothing) support
// - Real-time performance with <10μs ISR execution time

#ifndef AccelInterruptStepper_h
#define AccelInterruptStepper_h

#include <Arduino.h>
#include <stdint.h>

#define MAX_STEPPERS 4
#define SEGMENT_BUFFER_SIZE 8
#define DEFAULT_ISR_FREQUENCY 30000  // 30kHz base frequency

// Motor interface types (compatible with AccelStepper)
typedef enum {
    DRIVER    = 1, // Step/Direction pins
    FULL2WIRE = 2, // 2-wire stepper
    FULL4WIRE = 4  // 4-wire stepper
} MotorInterfaceType;

// Direction constants
typedef enum {
    DIRECTION_CCW = 0,
    DIRECTION_CW  = 1
} Direction;

// Stepper state for interrupt system
typedef struct {
    // Bresenham algorithm counters
    uint32_t counter;
    uint32_t steps_per_segment;
    
    // Position tracking
    volatile long current_pos;
    long target_pos;
    
    // Speed and acceleration
    float current_speed;     // steps/sec
    float max_speed;         // steps/sec  
    float acceleration;      // steps/sec²
    
    // Pin configuration
    uint8_t step_pin;
    uint8_t dir_pin;
    uint8_t enable_pin;
    uint8_t interface_type;
    
    // State flags
    volatile uint8_t step_bit;       // Output bit for this stepper in current segment
    uint8_t direction_bit;    // Direction bit
    boolean enabled;
    boolean pin_inverted[3];  // step, dir, enable inversion
    
    // Speed calculation variables (David Austin algorithm)
    long n;                   // Step counter for acceleration
    float c0;                 // Initial step interval
    float cn;                 // Current step interval 
    float cmin;               // Minimum step interval (max speed)
} stepper_state_t;

// Segment buffer for pre-computed motion segments
typedef struct {
    uint16_t n_steps;              // Number of steps in this segment
    uint16_t cycles_per_tick;      // Timer cycles per ISR tick
    uint32_t step_event_count;     // Total step events for Bresenham
    uint8_t stepper_bits[MAX_STEPPERS];  // Step bits for each stepper
    uint8_t direction_bits;        // Combined direction bits
    boolean active;
} motion_segment_t;

class AccelInterruptStepper {
public:
    // Constructor - similar to AccelStepper
    AccelInterruptStepper(uint8_t interface = DRIVER, 
                         uint8_t pin1 = 2, uint8_t pin2 = 3, 
                         uint8_t pin3 = 4, uint8_t pin4 = 5);
    
    // Destructor
    ~AccelInterruptStepper();
    
    // Static initialization (call once before using any steppers)
    static void begin();
    static void end();
    
    // Stepper management
    static uint8_t addStepper(AccelInterruptStepper* stepper);
    static boolean removeStepper(uint8_t stepper_id);
    
    // Motion control (AccelStepper compatible API)
    void moveTo(long absolute);
    void move(long relative);
    void setMaxSpeed(float speed);
    void setAcceleration(float acceleration);
    void setSpeed(float speed);
    void stop();
    
    // Status methods
    long currentPosition() const;
    long targetPosition() const;
    long distanceToGo() const;
    float speed() const;
    float maxSpeed() const;
    float acceleration() const;
    boolean isRunning() const;
    
    // Position control
    void setCurrentPosition(long position);
    
    // Pin control
    void setEnablePin(uint8_t pin);
    void setPinsInverted(bool step_invert = false, 
                        bool dir_invert = false, 
                        bool enable_invert = false);
    void enableOutputs();
    void disableOutputs();
    
    // Blocking motion (compatibility)
    void runToPosition();
    void runToNewPosition(long position);
    
    // Multi-stepper coordination
    static void moveToCoordinated(long positions[]); // Move all steppers coordinated
    static boolean runCoordinated();                 // Run coordinated motion
    static void waitForCompletion();                // Block until all motion complete
    
    // Advanced configuration
    static void setISRFrequency(uint32_t frequency);
    static void enableAMASS(boolean enable);
    
    // Status
    uint8_t getStepperID() const { return stepper_id; }
    static uint8_t getActiveSteppers() { return active_steppers; }
    
private:
    // Instance variables
    uint8_t stepper_id;
    static uint8_t next_stepper_id;
    static uint8_t active_steppers;
    
    // Static stepper management
    static stepper_state_t steppers[MAX_STEPPERS];
    static motion_segment_t segment_buffer[SEGMENT_BUFFER_SIZE];
    static volatile uint8_t segment_buffer_head;
    static volatile uint8_t segment_buffer_tail;
    
    // Interrupt system
    static volatile boolean interrupt_active;
    static volatile uint32_t isr_frequency;
    static volatile boolean amass_enabled;
    
    // Internal methods
    void computeSpeedProfile();
    static void computeBresenhamSegment();
    static boolean addSegmentToBuffer(motion_segment_t* segment);
    static motion_segment_t* getNextSegment();
    
    // Speed calculation methods (David Austin algorithm)
    unsigned long computeNewSpeed();
    void updateSpeed();
    
    // Hardware abstraction
    static void setupTimer1();
    static void configureStepperPins(uint8_t stepper_id);
    
    // Interrupt service routine
    static void stepperISR();
    
    // Friend declaration for ISR access
    friend void TIMER1_COMPA_vect_AccelInterrupt();
};

// Interrupt vector wrapper
ISR(TIMER1_COMPA_vect);

#endif