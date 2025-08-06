//
// OTPackInterruptExample.ino - Enhanced OT Pack using AccelInterruptStepper
//
// This example shows how to upgrade the existing OTPack system to use
// the new AccelInterruptStepper library for superior performance.
//
// Advantages over original OTPack:
// - No blocking delays - sensor can be monitored continuously
// - Precise timing regardless of other program activities  
// - Ready for multi-stepper expansion
// - Maintains the same simple interface

#include "AccelInterruptStepper.h"

// Hardware configuration - same as original OTPack
const byte SENSOR_INPUT_PIN = 3;
const byte STEPPER_STEP_PIN = 10;
const byte STEPPER_ENABLE_PIN = 9;
const byte STEPPER_DIRECTION_PIN = 8;

// Motor configuration - same as original
const int MICROSTEPPING_RESOLUTION = 4;
const int STEPS_PER_REVOLUTION = 58;
const int TOTAL_STEPS = STEPS_PER_REVOLUTION * MICROSTEPPING_RESOLUTION; // 232 steps

// Motion profiles - enhanced with interrupt system
const float FORWARD_MAX_SPEED = 1200.0 * MICROSTEPPING_RESOLUTION;   // 4800 steps/sec
const float FORWARD_ACCELERATION = 600.0 * MICROSTEPPING_RESOLUTION; // 2400 steps/sec²
const float REVERSE_MAX_SPEED = 3000.0 * MICROSTEPPING_RESOLUTION;   // 12000 steps/sec  
const float REVERSE_ACCELERATION = 1900.0 * MICROSTEPPING_RESOLUTION;// 7600 steps/sec²

// Timing parameters
const int FORWARD_DEBOUNCE_DELAY = 150;
const int REVERSE_DEBOUNCE_DELAY = 250;
const int REVERSE_SETTLE_DELAY = 100;
const int POSITION_OFFSET = 2;

// Create enhanced stepper instance
AccelInterruptStepper otpack_stepper(DRIVER, STEPPER_STEP_PIN, STEPPER_DIRECTION_PIN);

// State machine
enum OTPackState {
    WAITING_FOR_FORWARD,
    WAITING_FOR_REVERSE
};

OTPackState current_state = WAITING_FOR_FORWARD;
unsigned long last_sensor_change = 0;
boolean sensor_stable = true;

void setup() {
    Serial.begin(9600);
    Serial.println("Enhanced OTPack with AccelInterruptStepper");
    
    // Initialize interrupt stepper system
    AccelInterruptStepper::begin();
    
    // Configure sensor pin
    pinMode(SENSOR_INPUT_PIN, INPUT_PULLUP);
    pinMode(STEPPER_ENABLE_PIN, OUTPUT);
    
    // Configure stepper
    otpack_stepper.setMaxSpeed(FORWARD_MAX_SPEED);
    otpack_stepper.setAcceleration(FORWARD_ACCELERATION);
    otpack_stepper.setEnablePin(STEPPER_ENABLE_PIN);
    otpack_stepper.enableOutputs();
    
    // Add stepper to interrupt system
    AccelInterruptStepper::addStepper(&otpack_stepper);
    
    Serial.println("Enhanced OTPack ready");
    Serial.println("Sensor monitoring with interrupt-driven stepper");
}

void loop() {
    // Read sensor state
    boolean sensor_state = digitalRead(SENSOR_INPUT_PIN);
    
    // Continuous sensor monitoring (enhanced!)
    Serial.print("| digitalRead(3): ");
    Serial.print(sensor_state);
    Serial.print(" | Position: ");
    Serial.print(otpack_stepper.currentPosition());
    Serial.print(" | Speed: ");
    Serial.print(otpack_stepper.speed());
    Serial.print(" | State: ");
    Serial.println(current_state == WAITING_FOR_FORWARD ? "FORWARD" : "REVERSE");
    
    // Enhanced state machine with interrupt stepper
    switch (current_state) {
        case WAITING_FOR_FORWARD:
            if (sensor_state == HIGH && sensor_stable) {
                executeForwardMotion();
            }
            break;
            
        case WAITING_FOR_REVERSE:
            if (sensor_state == LOW && sensor_stable) {
                executeReverseMotion();
            }
            break;
    }
    
    // Update sensor stability
    updateSensorStability(sensor_state);
    
    // Simulate other tasks that would block original OTPack
    performOtherTasks();
    
    delay(50); // Faster monitoring possible due to non-blocking stepper
}

void executeForwardMotion() {
    Serial.println("=== Forward Motion Triggered ===");
    
    // Configure for forward motion
    otpack_stepper.setMaxSpeed(FORWARD_MAX_SPEED);
    otpack_stepper.setAcceleration(FORWARD_ACCELERATION);
    
    // Debounce delay
    delay(FORWARD_DEBOUNCE_DELAY);
    
    // Enable motor
    digitalWrite(STEPPER_ENABLE_PIN, LOW);
    
    // Execute motion (non-blocking!)
    otpack_stepper.move(TOTAL_STEPS);
    
    Serial.print("Forward motion started: ");
    Serial.print(TOTAL_STEPS);
    Serial.println(" steps");
    
    // Wait for motion completion while doing other tasks
    waitForMotionWithTasks();
    
    // Transition state
    current_state = WAITING_FOR_REVERSE;
    Serial.println("=== Forward Motion Complete ===");
}

void executeReverseMotion() {
    Serial.println("=== Reverse Motion Triggered ===");
    
    // Configure for reverse motion
    otpack_stepper.setMaxSpeed(REVERSE_MAX_SPEED);
    otpack_stepper.setAcceleration(REVERSE_ACCELERATION);
    
    // Debounce delay
    delay(REVERSE_DEBOUNCE_DELAY);
    
    // Execute motion with position offset (non-blocking!)
    otpack_stepper.move(-(TOTAL_STEPS - POSITION_OFFSET));
    
    Serial.print("Reverse motion started: ");
    Serial.print(-(TOTAL_STEPS - POSITION_OFFSET));
    Serial.println(" steps");
    
    // Wait for motion completion while doing other tasks
    waitForMotionWithTasks();
    
    // Settle delay
    delay(REVERSE_SETTLE_DELAY);
    
    // Disable motor
    digitalWrite(STEPPER_ENABLE_PIN, HIGH);
    
    // Transition state
    current_state = WAITING_FOR_FORWARD;
    Serial.println("=== Reverse Motion Complete ===");
}

void waitForMotionWithTasks() {
    Serial.println("Waiting for motion (performing other tasks...)");
    
    unsigned long task_counter = 0;
    
    while (otpack_stepper.isRunning()) {
        // Perform other tasks while stepper moves
        task_counter++;
        
        // Simulate sensor monitoring
        boolean sensor = digitalRead(SENSOR_INPUT_PIN);
        
        // Simulate data processing
        float calculated_value = sin(millis() * 0.001) * 100;
        
        // Simulate communication every 500ms
        if (millis() % 500 < 50) {
            Serial.print("  Task #"); Serial.print(task_counter);
            Serial.print(" | Pos: "); Serial.print(otpack_stepper.currentPosition());
            Serial.print(" | Sensor: "); Serial.print(sensor);
            Serial.print(" | Calc: "); Serial.println(calculated_value);
        }
        
        // This would block original OTPack, but not enhanced version!
        delay(10);
        
        // Check for emergency conditions
        if (Serial.available()) {
            char cmd = Serial.read();
            if (cmd == 's' || cmd == 'S') {
                Serial.println("EMERGENCY STOP!");
                otpack_stepper.stop();
                break;
            }
        }
    }
    
    Serial.print("Motion complete after ");
    Serial.print(task_counter);
    Serial.println(" task cycles");
}

void updateSensorStability(boolean current_sensor_state) {
    static boolean last_sensor_state = LOW;
    
    if (current_sensor_state != last_sensor_state) {
        last_sensor_change = millis();
        sensor_stable = false;
        last_sensor_state = current_sensor_state;
    } else {
        // Check if sensor has been stable long enough
        if (!sensor_stable && (millis() - last_sensor_change > 50)) {
            sensor_stable = true;
        }
    }
}

void performOtherTasks() {
    // Simulate other system tasks that would interfere with original OTPack
    
    // Task 1: Analog reading and processing
    int analog_value = analogRead(A0);
    float processed = analog_value * 3.3 / 1024.0;
    
    // Task 2: Serial communication handling
    if (Serial.available()) {
        String input = Serial.readString();
        input.trim();
        handleSerialCommand(input);
    }
    
    // Task 3: LED blinking or other visual feedback
    static unsigned long last_blink = 0;
    if (millis() - last_blink > 1000) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        last_blink = millis();
    }
    
    // Task 4: System monitoring
    static unsigned long last_monitor = 0;
    if (millis() - last_monitor > 5000) {
        printSystemStatus();
        last_monitor = millis();
    }
}

void handleSerialCommand(String command) {
    command.toLowerCase();
    
    if (command == "status") {
        printSystemStatus();
    } else if (command == "stop") {
        otpack_stepper.stop();
        Serial.println("Stepper stopped");
    } else if (command == "reset") {
        otpack_stepper.setCurrentPosition(0);
        current_state = WAITING_FOR_FORWARD;
        Serial.println("System reset");
    } else if (command == "test") {
        performTestSequence();
    } else if (command == "help") {
        printHelp();
    } else {
        Serial.println("Unknown command. Type 'help' for commands.");
    }
}

void printSystemStatus() {
    Serial.println("\n=== Enhanced OTPack Status ===");
    Serial.print("State: "); 
    Serial.println(current_state == WAITING_FOR_FORWARD ? "WAITING_FOR_FORWARD" : "WAITING_FOR_REVERSE");
    Serial.print("Position: "); Serial.println(otpack_stepper.currentPosition());
    Serial.print("Target: "); Serial.println(otpack_stepper.targetPosition());
    Serial.print("Speed: "); Serial.print(otpack_stepper.speed()); Serial.println(" steps/sec");
    Serial.print("Is Running: "); Serial.println(otpack_stepper.isRunning() ? "Yes" : "No");
    Serial.print("Sensor: "); Serial.println(digitalRead(SENSOR_INPUT_PIN) ? "HIGH" : "LOW");
    Serial.print("Free RAM: "); Serial.print(getFreeRAM()); Serial.println(" bytes");
    Serial.println("==============================\n");
}

void performTestSequence() {
    Serial.println("Performing test sequence...");
    
    // Test forward motion
    Serial.println("Test: Forward motion");
    otpack_stepper.setMaxSpeed(FORWARD_MAX_SPEED);
    otpack_stepper.setAcceleration(FORWARD_ACCELERATION);
    otpack_stepper.move(TOTAL_STEPS);
    waitForMotionWithTasks();
    
    delay(1000);
    
    // Test reverse motion  
    Serial.println("Test: Reverse motion");
    otpack_stepper.setMaxSpeed(REVERSE_MAX_SPEED);
    otpack_stepper.setAcceleration(REVERSE_ACCELERATION);
    otpack_stepper.move(-TOTAL_STEPS);
    waitForMotionWithTasks();
    
    Serial.println("Test sequence complete");
}

void printHelp() {
    Serial.println("\n=== Enhanced OTPack Commands ===");
    Serial.println("status  - Show system status");
    Serial.println("stop    - Emergency stop");
    Serial.println("reset   - Reset to initial state");
    Serial.println("test    - Run test sequence");
    Serial.println("help    - Show this help");
    Serial.println("=================================\n");
}

int getFreeRAM() {
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

// Performance comparison function
void comparePerformance() {
    Serial.println("\n=== Performance Comparison ===");
    Serial.println("Original OTPack vs Enhanced OTPack:");
    Serial.println("");
    Serial.println("Original OTPack:");
    Serial.println("- Blocking runToPosition() calls");
    Serial.println("- Sensor monitoring stops during motion");
    Serial.println("- No multitasking capability");
    Serial.println("- Motion can be delayed by other code");
    Serial.println("");
    Serial.println("Enhanced OTPack (AccelInterruptStepper):");
    Serial.println("- Non-blocking interrupt-driven motion");
    Serial.println("- Continuous sensor monitoring");
    Serial.println("- Full multitasking capability");
    Serial.println("- Precise timing regardless of other tasks");
    Serial.println("- Ready for multi-stepper expansion");
    Serial.println("- Same familiar API");
    Serial.println("==============================\n");
}