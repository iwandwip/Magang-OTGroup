//
// AccelInterruptStepper.cpp - Implementation combining AccelStepper flexibility with GRBL interrupt system
//
// Copyright (C) 2024 AccelInterruptStepper Project
// Based on AccelStepper by Mike McCauley and GRBL by Sungeun K. Jeon

#include "AccelInterruptStepper.h"
#include <avr/interrupt.h>
#include <math.h>

// Static member initialization
uint8_t AccelInterruptStepper::next_stepper_id = 0;
uint8_t AccelInterruptStepper::active_steppers = 0;
stepper_state_t AccelInterruptStepper::steppers[MAX_STEPPERS];
motion_segment_t AccelInterruptStepper::segment_buffer[SEGMENT_BUFFER_SIZE];
volatile uint8_t AccelInterruptStepper::segment_buffer_head = 0;
volatile uint8_t AccelInterruptStepper::segment_buffer_tail = 0;
volatile boolean AccelInterruptStepper::interrupt_active = false;
volatile uint32_t AccelInterruptStepper::isr_frequency = DEFAULT_ISR_FREQUENCY;
volatile boolean AccelInterruptStepper::amass_enabled = true;

// Hardware timer configuration
#define TIMER1_PRESCALER_1   (1<<CS10)
#define TIMER1_PRESCALER_8   (1<<CS11)
#define TIMER1_PRESCALER_64  (1<<CS11)|(1<<CS10)
#define TIMER1_PRESCALER_256 (1<<CS12)
#define TIMER1_PRESCALER_1024 (1<<CS12)|(1<<CS10)

// Step and direction pin masks for fast port access
volatile uint8_t step_port_mask = 0;
volatile uint8_t dir_port_mask = 0;

// Interrupt state
static volatile struct {
    uint32_t step_event_count;
    uint32_t counters[MAX_STEPPERS];
    uint8_t step_bits;
    uint8_t dir_bits;
    uint16_t segment_steps_remaining;
    motion_segment_t* current_segment;
    boolean busy;
} isr_state;

// Constructor
AccelInterruptStepper::AccelInterruptStepper(uint8_t interface, uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4) {
    if (next_stepper_id >= MAX_STEPPERS) {
        stepper_id = 0xFF; // Invalid ID
        return;
    }
    
    stepper_id = next_stepper_id++;
    stepper_state_t* state = &steppers[stepper_id];
    
    // Initialize stepper state
    state->current_pos = 0;
    state->target_pos = 0;
    state->current_speed = 0.0;
    state->max_speed = 1.0;
    state->acceleration = 1.0;
    state->interface_type = interface;
    state->enabled = false;
    
    // Configure pins based on interface type
    switch (interface) {
        case DRIVER:
            state->step_pin = pin1;
            state->dir_pin = pin2;
            state->enable_pin = 0xFF;
            break;
        case FULL2WIRE:
            state->step_pin = pin1;
            state->dir_pin = pin2;
            state->enable_pin = 0xFF;
            break;
        case FULL4WIRE:
            // For 4-wire, we'll use pins differently
            state->step_pin = pin1;
            state->dir_pin = pin2;
            state->enable_pin = 0xFF;
            break;
    }
    
    // Initialize speed calculation variables (David Austin algorithm)
    state->n = 0;
    state->c0 = 0.0;
    state->cn = 0.0;
    state->cmin = 1000000.0; // 1 step per second initially
    
    // Pin inversion defaults
    state->pin_inverted[0] = false; // step
    state->pin_inverted[1] = false; // dir
    state->pin_inverted[2] = false; // enable
    
    // Configure hardware pins
    configureStepperPins(stepper_id);
}

// Destructor
AccelInterruptStepper::~AccelInterruptStepper() {
    removeStepper(stepper_id);
}

// Static initialization
void AccelInterruptStepper::begin() {
    if (interrupt_active) return;
    
    // Initialize all steppers to inactive
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        steppers[i].enabled = false;
        steppers[i].step_bit = 0;
        steppers[i].direction_bit = 0;
    }
    
    // Initialize segment buffer
    segment_buffer_head = 0;
    segment_buffer_tail = 0;
    for (uint8_t i = 0; i < SEGMENT_BUFFER_SIZE; i++) {
        segment_buffer[i].active = false;
    }
    
    // Initialize interrupt state
    isr_state.step_event_count = 0;
    isr_state.step_bits = 0;
    isr_state.dir_bits = 0;
    isr_state.segment_steps_remaining = 0;
    isr_state.current_segment = NULL;
    isr_state.busy = false;
    
    // Setup Timer1 for interrupt generation
    setupTimer1();
    
    interrupt_active = true;
}

// Static cleanup
void AccelInterruptStepper::end() {
    if (!interrupt_active) return;
    
    // Disable Timer1 interrupt
    TIMSK1 &= ~(1<<OCIE1A);
    
    // Reset Timer1
    TCCR1A = 0;
    TCCR1B = 0;
    OCR1A = 0;
    
    interrupt_active = false;
    active_steppers = 0;
    next_stepper_id = 0;
}

// Add stepper to interrupt system
uint8_t AccelInterruptStepper::addStepper(AccelInterruptStepper* stepper) {
    if (!interrupt_active || stepper->stepper_id >= MAX_STEPPERS) {
        return 0xFF; // Error
    }
    
    steppers[stepper->stepper_id].enabled = true;
    active_steppers++;
    
    return stepper->stepper_id;
}

// Remove stepper from interrupt system
boolean AccelInterruptStepper::removeStepper(uint8_t stepper_id) {
    if (stepper_id >= MAX_STEPPERS || !steppers[stepper_id].enabled) {
        return false;
    }
    
    steppers[stepper_id].enabled = false;
    if (active_steppers > 0) active_steppers--;
    
    return true;
}

// Motion control methods
void AccelInterruptStepper::moveTo(long absolute) {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    if (state->target_pos != absolute) {
        state->target_pos = absolute;
        computeSpeedProfile();
    }
}

void AccelInterruptStepper::move(long relative) {
    moveTo(currentPosition() + relative);
}

void AccelInterruptStepper::setMaxSpeed(float speed) {
    if (stepper_id >= MAX_STEPPERS || speed <= 0.0) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    if (state->max_speed != speed) {
        state->max_speed = speed;
        state->cmin = 1000000.0 / speed; // Convert to microseconds
        
        // Recompute if currently accelerating
        if (state->n > 0) {
            state->n = (long)((state->current_speed * state->current_speed) / (2.0 * state->acceleration));
            computeSpeedProfile();
        }
    }
}

void AccelInterruptStepper::setAcceleration(float acceleration) {
    if (stepper_id >= MAX_STEPPERS || acceleration <= 0.0) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    if (state->acceleration != acceleration) {
        // Recompute n per equation from David Austin algorithm
        if (state->acceleration > 0.0) {
            state->n = state->n * (state->acceleration / acceleration);
        }
        
        // New c0 per Equation 7, with correction per Equation 15
        state->c0 = 0.676 * sqrt(2.0 / acceleration) * 1000000.0;
        state->acceleration = acceleration;
        computeSpeedProfile();
    }
}

void AccelInterruptStepper::setSpeed(float speed) {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    speed = constrain(speed, -state->max_speed, state->max_speed);
    state->current_speed = speed;
}

void AccelInterruptStepper::stop() {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    if (state->current_speed != 0.0) {
        long stepsToStop = (long)((state->current_speed * state->current_speed) / (2.0 * state->acceleration)) + 1;
        if (state->current_speed > 0) {
            move(stepsToStop);
        } else {
            move(-stepsToStop);
        }
    }
}

// Status methods
long AccelInterruptStepper::currentPosition() const {
    if (stepper_id >= MAX_STEPPERS) return 0;
    return steppers[stepper_id].current_pos;
}

long AccelInterruptStepper::targetPosition() const {
    if (stepper_id >= MAX_STEPPERS) return 0;
    return steppers[stepper_id].target_pos;
}

long AccelInterruptStepper::distanceToGo() const {
    return targetPosition() - currentPosition();
}

float AccelInterruptStepper::speed() const {
    if (stepper_id >= MAX_STEPPERS) return 0.0;
    return steppers[stepper_id].current_speed;
}

float AccelInterruptStepper::maxSpeed() const {
    if (stepper_id >= MAX_STEPPERS) return 0.0;
    return steppers[stepper_id].max_speed;
}

float AccelInterruptStepper::acceleration() const {
    if (stepper_id >= MAX_STEPPERS) return 0.0;
    return steppers[stepper_id].acceleration;
}

boolean AccelInterruptStepper::isRunning() const {
    if (stepper_id >= MAX_STEPPERS) return false;
    stepper_state_t* state = &steppers[stepper_id];
    return !(state->current_speed == 0.0 && state->target_pos == state->current_pos);
}

void AccelInterruptStepper::setCurrentPosition(long position) {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    state->target_pos = state->current_pos = position;
    state->n = 0;
    state->current_speed = 0.0;
}

// Pin control
void AccelInterruptStepper::setEnablePin(uint8_t pin) {
    if (stepper_id >= MAX_STEPPERS) return;
    
    steppers[stepper_id].enable_pin = pin;
    if (pin != 0xFF) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, steppers[stepper_id].pin_inverted[2] ? LOW : HIGH);
    }
}

void AccelInterruptStepper::setPinsInverted(bool step_invert, bool dir_invert, bool enable_invert) {
    if (stepper_id >= MAX_STEPPERS) return;
    
    steppers[stepper_id].pin_inverted[0] = step_invert;
    steppers[stepper_id].pin_inverted[1] = dir_invert;
    steppers[stepper_id].pin_inverted[2] = enable_invert;
}

void AccelInterruptStepper::enableOutputs() {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    
    // Configure pins as outputs
    pinMode(state->step_pin, OUTPUT);
    pinMode(state->dir_pin, OUTPUT);
    
    if (state->enable_pin != 0xFF) {
        pinMode(state->enable_pin, OUTPUT);
        digitalWrite(state->enable_pin, state->pin_inverted[2] ? LOW : HIGH);
    }
}

void AccelInterruptStepper::disableOutputs() {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    
    // Set step and direction pins low
    digitalWrite(state->step_pin, state->pin_inverted[0] ? HIGH : LOW);
    digitalWrite(state->dir_pin, state->pin_inverted[1] ? HIGH : LOW);
    
    if (state->enable_pin != 0xFF) {
        digitalWrite(state->enable_pin, state->pin_inverted[2] ? HIGH : LOW);
    }
}

// Blocking motion
void AccelInterruptStepper::runToPosition() {
    while (isRunning()) {
        yield(); // Allow other tasks
        delay(1);
    }
}

void AccelInterruptStepper::runToNewPosition(long position) {
    moveTo(position);
    runToPosition();
}

// Multi-stepper coordination
void AccelInterruptStepper::moveToCoordinated(long positions[]) {
    // Find the stepper that needs to travel the longest distance
    float max_distance = 0;
    float distances[MAX_STEPPERS];
    
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if (steppers[i].enabled) {
            distances[i] = abs(positions[i] - steppers[i].current_pos);
            if (distances[i] > max_distance) {
                max_distance = distances[i];
            }
        }
    }
    
    if (max_distance == 0) return; // No movement needed
    
    // Calculate proportional speeds for coordinated motion
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if (steppers[i].enabled && distances[i] > 0) {
            float speed_ratio = distances[i] / max_distance;
            steppers[i].current_speed = steppers[i].max_speed * speed_ratio;
            steppers[i].target_pos = positions[i];
        }
    }
    
    // Generate coordinated motion segment
    computeBresenhamSegment();
}

boolean AccelInterruptStepper::runCoordinated() {
    boolean any_running = false;
    
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if (steppers[i].enabled && steppers[i].current_pos != steppers[i].target_pos) {
            any_running = true;
            break;
        }
    }
    
    return any_running;
}

void AccelInterruptStepper::waitForCompletion() {
    while (runCoordinated()) {
        yield();
        delay(1);
    }
}

// Advanced configuration
void AccelInterruptStepper::setISRFrequency(uint32_t frequency) {
    if (frequency < 1000 || frequency > 50000) return; // Reasonable limits
    
    isr_frequency = frequency;
    if (interrupt_active) {
        setupTimer1(); // Reconfigure timer
    }
}

void AccelInterruptStepper::enableAMASS(boolean enable) {
    amass_enabled = enable;
}

// Private methods
void AccelInterruptStepper::computeSpeedProfile() {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    long distanceTo = state->target_pos - state->current_pos;
    
    long stepsToStop = (long)((state->current_speed * state->current_speed) / (2.0 * state->acceleration));
    
    if (distanceTo == 0 && stepsToStop <= 1) {
        // At target, stop
        state->current_speed = 0.0;
        state->n = 0;
        return;
    }
    
    // Implement David Austin's speed profile algorithm
    if (distanceTo > 0) {
        // Moving clockwise
        if (state->n > 0) {
            if ((stepsToStop >= distanceTo) || state->current_speed < 0) {
                state->n = -stepsToStop; // Start deceleration
            }
        } else if (state->n < 0) {
            if ((stepsToStop < distanceTo) && state->current_speed >= 0) {
                state->n = -state->n; // Start acceleration
            }
        }
    } else if (distanceTo < 0) {
        // Moving counter-clockwise
        if (state->n > 0) {
            if ((stepsToStop >= -distanceTo) || state->current_speed > 0) {
                state->n = -stepsToStop; // Start deceleration
            }
        } else if (state->n < 0) {
            if ((stepsToStop < -distanceTo) && state->current_speed <= 0) {
                state->n = -state->n; // Start acceleration
            }
        }
    }
    
    // Calculate new speed
    if (state->n == 0) {
        // First step from stopped
        state->cn = state->c0;
    } else {
        // Subsequent step - David Austin equation
        state->cn = state->cn - ((2.0 * state->cn) / ((4.0 * state->n) + 1));
        state->cn = max(state->cn, state->cmin);
    }
    
    state->n++;
    state->current_speed = 1000000.0 / state->cn;
    if (distanceTo < 0) {
        state->current_speed = -state->current_speed;
    }
}

void AccelInterruptStepper::computeBresenhamSegment() {
    // Find next available segment buffer slot
    uint8_t next_head = (segment_buffer_head + 1) % SEGMENT_BUFFER_SIZE;
    if (next_head == segment_buffer_tail) {
        return; // Buffer full
    }
    
    motion_segment_t* segment = &segment_buffer[segment_buffer_head];
    
    // Find the dominant axis (longest travel distance)
    uint32_t max_steps = 0;
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if (steppers[i].enabled) {
            uint32_t steps = abs(steppers[i].target_pos - steppers[i].current_pos);
            if (steps > max_steps) {
                max_steps = steps;
            }
        }
    }
    
    if (max_steps == 0) return;
    
    // Configure segment
    segment->n_steps = min(max_steps, 255); // Limit segment size
    segment->step_event_count = max_steps;
    segment->cycles_per_tick = F_CPU / isr_frequency;
    segment->direction_bits = 0;
    segment->active = true;
    
    // Set direction bits and step counts for each stepper
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if (steppers[i].enabled) {
            long distance = steppers[i].target_pos - steppers[i].current_pos;
            if (distance > 0) {
                segment->direction_bits |= (1 << i);
                segment->stepper_bits[i] = abs(distance);
            } else if (distance < 0) {
                segment->stepper_bits[i] = abs(distance);
            } else {
                segment->stepper_bits[i] = 0;
            }
        } else {
            segment->stepper_bits[i] = 0;
        }
    }
    
    // Add segment to buffer
    segment_buffer_head = next_head;
}

boolean AccelInterruptStepper::addSegmentToBuffer(motion_segment_t* segment) {
    uint8_t next_head = (segment_buffer_head + 1) % SEGMENT_BUFFER_SIZE;
    if (next_head == segment_buffer_tail) {
        return false; // Buffer full
    }
    
    segment_buffer[segment_buffer_head] = *segment;
    segment_buffer_head = next_head;
    return true;
}

motion_segment_t* AccelInterruptStepper::getNextSegment() {
    if (segment_buffer_head == segment_buffer_tail) {
        return NULL; // Buffer empty
    }
    
    motion_segment_t* segment = &segment_buffer[segment_buffer_tail];
    return segment;
}

void AccelInterruptStepper::setupTimer1() {
    // Calculate Timer1 configuration for desired ISR frequency
    // Using CTC mode (Clear Timer on Compare Match)
    
    noInterrupts();
    
    // Reset Timer1
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    
    // Calculate prescaler and compare value
    uint32_t compare_value;
    uint8_t prescaler_bits;
    
    if (isr_frequency <= 250) {
        // Use 1024 prescaler
        compare_value = F_CPU / (1024UL * isr_frequency) - 1;
        prescaler_bits = TIMER1_PRESCALER_1024;
    } else if (isr_frequency <= 2000) {
        // Use 256 prescaler
        compare_value = F_CPU / (256UL * isr_frequency) - 1;
        prescaler_bits = TIMER1_PRESCALER_256;
    } else if (isr_frequency <= 16000) {
        // Use 64 prescaler
        compare_value = F_CPU / (64UL * isr_frequency) - 1;
        prescaler_bits = TIMER1_PRESCALER_64;
    } else {
        // Use 8 prescaler
        compare_value = F_CPU / (8UL * isr_frequency) - 1;
        prescaler_bits = TIMER1_PRESCALER_8;
    }
    
    // Limit compare value to 16-bit
    if (compare_value > 65535) compare_value = 65535;
    
    // Configure Timer1 for CTC mode
    OCR1A = compare_value;
    TCCR1B = (1<<WGM12) | prescaler_bits; // CTC mode + prescaler
    
    // Enable Timer1 compare interrupt
    TIMSK1 |= (1<<OCIE1A);
    
    interrupts();
}

void AccelInterruptStepper::configureStepperPins(uint8_t stepper_id) {
    if (stepper_id >= MAX_STEPPERS) return;
    
    stepper_state_t* state = &steppers[stepper_id];
    
    // Configure pins based on interface type
    switch (state->interface_type) {
        case DRIVER:
            pinMode(state->step_pin, OUTPUT);
            pinMode(state->dir_pin, OUTPUT);
            digitalWrite(state->step_pin, state->pin_inverted[0] ? HIGH : LOW);
            digitalWrite(state->dir_pin, state->pin_inverted[1] ? HIGH : LOW);
            break;
            
        case FULL2WIRE:
        case FULL4WIRE:
            pinMode(state->step_pin, OUTPUT);
            pinMode(state->dir_pin, OUTPUT);
            digitalWrite(state->step_pin, LOW);
            digitalWrite(state->dir_pin, LOW);
            break;
    }
    
    if (state->enable_pin != 0xFF) {
        pinMode(state->enable_pin, OUTPUT);
        digitalWrite(state->enable_pin, state->pin_inverted[2] ? LOW : HIGH);
    }
}

// Main interrupt service routine
void AccelInterruptStepper::stepperISR() {
    if (isr_state.busy) return; // Prevent re-entry
    isr_state.busy = true;
    
    // Get current segment or load new one
    if (isr_state.current_segment == NULL || isr_state.segment_steps_remaining == 0) {
        isr_state.current_segment = getNextSegment();
        if (isr_state.current_segment == NULL) {
            isr_state.busy = false;
            return; // No segments to execute
        }
        
        // Initialize new segment
        isr_state.step_event_count = isr_state.current_segment->step_event_count;
        isr_state.segment_steps_remaining = isr_state.current_segment->n_steps;
        
        // Initialize Bresenham counters
        for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
            isr_state.counters[i] = isr_state.step_event_count >> 1; // Start at half step
        }
        
        // Set direction pins first
        for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
            if (steppers[i].enabled) {
                boolean dir = (isr_state.current_segment->direction_bits & (1 << i)) != 0;
                digitalWrite(steppers[i].dir_pin, 
                           dir ? (steppers[i].pin_inverted[1] ? LOW : HIGH) 
                               : (steppers[i].pin_inverted[1] ? HIGH : LOW));
            }
        }
    }
    
    // Execute Bresenham algorithm for each stepper
    isr_state.step_bits = 0;
    
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if (steppers[i].enabled && isr_state.current_segment->stepper_bits[i] > 0) {
            isr_state.counters[i] += isr_state.current_segment->stepper_bits[i];
            
            if (isr_state.counters[i] > isr_state.step_event_count) {
                // Step this stepper
                isr_state.step_bits |= (1 << i);
                isr_state.counters[i] -= isr_state.step_event_count;
                
                // Update position
                if (isr_state.current_segment->direction_bits & (1 << i)) {
                    steppers[i].current_pos++;
                } else {
                    steppers[i].current_pos--;
                }
            }
        }
    }
    
    // Generate step pulses
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if ((isr_state.step_bits & (1 << i)) && steppers[i].enabled) {
            // Step pulse HIGH
            digitalWrite(steppers[i].step_pin, steppers[i].pin_inverted[0] ? LOW : HIGH);
        }
    }
    
    // Wait minimum pulse width (2μs)
    delayMicroseconds(2);
    
    // Step pulse LOW
    for (uint8_t i = 0; i < MAX_STEPPERS; i++) {
        if ((isr_state.step_bits & (1 << i)) && steppers[i].enabled) {
            digitalWrite(steppers[i].step_pin, steppers[i].pin_inverted[0] ? HIGH : LOW);
        }
    }
    
    // Update segment progress
    if (isr_state.segment_steps_remaining > 0) {
        isr_state.segment_steps_remaining--;
    }
    
    // Check if segment completed
    if (isr_state.segment_steps_remaining == 0) {
        // Move to next segment
        segment_buffer_tail = (segment_buffer_tail + 1) % SEGMENT_BUFFER_SIZE;
        isr_state.current_segment = NULL;
    }
    
    isr_state.busy = false;
}

// Interrupt vector
ISR(TIMER1_COMPA_vect) {
    AccelInterruptStepper::stepperISR();
}