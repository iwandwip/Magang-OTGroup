//
// AccelInterruptStepperExample.ino - Example usage of AccelInterruptStepper library
//
// This example demonstrates the enhanced stepper library that combines:
// - AccelStepper's rich API and David Austin's speed algorithm
// - GRBL's interrupt-based multi-stepper Bresenham algorithm
//
// Features demonstrated:
// - Single stepper control (similar to AccelStepper)
// - Multi-stepper coordinated motion
// - Interrupt-driven operation (no blocking)
// - Real-time performance

#include "AccelInterruptStepper.h"

// Create stepper instances - similar to AccelStepper API
AccelInterruptStepper stepper1(DRIVER, 2, 3);    // Step pin 2, Dir pin 3
AccelInterruptStepper stepper2(DRIVER, 4, 5);    // Step pin 4, Dir pin 5
AccelInterruptStepper stepper3(DRIVER, 6, 7);    // Step pin 6, Dir pin 7
AccelInterruptStepper stepper4(DRIVER, 8, 9);    // Step pin 8, Dir pin 9

void setup() {
    Serial.begin(115200);
    Serial.println("AccelInterruptStepper Example");
    
    // Initialize the interrupt system (call ONCE)
    AccelInterruptStepper::begin();
    
    // Configure each stepper
    stepper1.setMaxSpeed(2000);     // 2000 steps/sec
    stepper1.setAcceleration(1000); // 1000 steps/sec²
    stepper1.enableOutputs();
    
    stepper2.setMaxSpeed(1500);
    stepper2.setAcceleration(800);
    stepper2.enableOutputs();
    
    stepper3.setMaxSpeed(1800);
    stepper3.setAcceleration(900);
    stepper3.enableOutputs();
    
    stepper4.setMaxSpeed(2200);
    stepper4.setAcceleration(1100);
    stepper4.enableOutputs();
    
    // Add steppers to interrupt system
    AccelInterruptStepper::addStepper(&stepper1);
    AccelInterruptStepper::addStepper(&stepper2);
    AccelInterruptStepper::addStepper(&stepper3);
    AccelInterruptStepper::addStepper(&stepper4);
    
    Serial.println("Setup complete. Starting motion examples...");
    delay(2000);
}

void loop() {
    // Example 1: Individual stepper movements (AccelStepper-style)
    Serial.println("Example 1: Individual movements");
    individualMovements();
    delay(3000);
    
    // Example 2: Coordinated multi-stepper motion (GRBL-style)
    Serial.println("Example 2: Coordinated motion");
    coordinatedMotion();
    delay(3000);
    
    // Example 3: Complex motion patterns
    Serial.println("Example 3: Complex patterns");
    complexPatterns();
    delay(3000);
    
    // Example 4: Real-time control demonstration
    Serial.println("Example 4: Real-time control");
    realTimeControl();
    delay(3000);
}

// Example 1: Individual stepper movements (AccelStepper compatibility)
void individualMovements() {
    Serial.println("  Moving steppers individually...");
    
    // Move each stepper to different positions
    stepper1.moveTo(1000);   // Move stepper1 to position 1000
    stepper2.moveTo(800);    // Move stepper2 to position 800
    stepper3.moveTo(-600);   // Move stepper3 to position -600
    stepper4.moveTo(1200);   // Move stepper4 to position 1200
    
    // Wait for all movements to complete
    while (stepper1.isRunning() || stepper2.isRunning() || 
           stepper3.isRunning() || stepper4.isRunning()) {
        
        // Print status while moving (non-blocking!)
        Serial.print("Positions: S1="); Serial.print(stepper1.currentPosition());
        Serial.print(" S2="); Serial.print(stepper2.currentPosition());
        Serial.print(" S3="); Serial.print(stepper3.currentPosition());
        Serial.print(" S4="); Serial.println(stepper4.currentPosition());
        
        delay(500); // This delay doesn't affect stepper motion!
    }
    
    Serial.println("  Individual movements complete");
}

// Example 2: Coordinated multi-stepper motion (GRBL-style)
void coordinatedMotion() {
    Serial.println("  Moving steppers in coordination...");
    
    // Define target positions for coordinated motion
    long target_positions[4] = {500, -400, 800, -600};
    
    // Move all steppers coordinated (they arrive at same time)
    AccelInterruptStepper::moveToCoordinated(target_positions);
    
    // Wait for coordinated motion to complete
    AccelInterruptStepper::waitForCompletion();
    
    Serial.println("  Coordinated motion complete");
}

// Example 3: Complex motion patterns
void complexPatterns() {
    Serial.println("  Executing complex patterns...");
    
    // Pattern 1: Square motion (2 steppers forming X-Y)
    Serial.println("    Square pattern...");
    long square_points[][2] = {
        {0, 0}, {400, 0}, {400, 400}, {0, 400}, {0, 0}
    };
    
    for (int i = 0; i < 5; i++) {
        long coords[4] = {square_points[i][0], square_points[i][1], 0, 0};
        AccelInterruptStepper::moveToCoordinated(coords);
        AccelInterruptStepper::waitForCompletion();
        delay(500);
    }
    
    // Pattern 2: Circular motion approximation
    Serial.println("    Circular pattern...");
    int radius = 300;
    for (int angle = 0; angle < 360; angle += 30) {
        float rad = angle * PI / 180.0;
        long x = radius * cos(rad);
        long y = radius * sin(rad);
        
        long coords[4] = {x, y, 0, 0};
        AccelInterruptStepper::moveToCoordinated(coords);
        AccelInterruptStepper::waitForCompletion();
        delay(200);
    }
}

// Example 4: Real-time control demonstration
void realTimeControl() {
    Serial.println("  Demonstrating real-time control...");
    Serial.println("  (Steppers move while other tasks execute)");
    
    // Start continuous motion
    stepper1.moveTo(2000);
    stepper2.moveTo(-1500);
    stepper3.moveTo(1800);
    stepper4.moveTo(-2200);
    
    // Simulate other real-time tasks
    unsigned long start_time = millis();
    int sensor_readings = 0;
    
    while (stepper1.isRunning() || stepper2.isRunning() || 
           stepper3.isRunning() || stepper4.isRunning()) {
        
        // Simulate sensor reading
        int sensor_value = analogRead(A0);
        sensor_readings++;
        
        // Simulate data processing
        float processed_value = sensor_value * 0.5 + sin(millis() * 0.001) * 100;
        
        // Simulate communication
        if (millis() % 1000 < 50) {
            Serial.print("  Sensor readings: "); Serial.print(sensor_readings);
            Serial.print(" | Processed: "); Serial.print(processed_value);
            Serial.print(" | S1 pos: "); Serial.println(stepper1.currentPosition());
        }
        
        // Simulate PID control or other time-critical tasks
        delayMicroseconds(100); // 100μs task
        
        // The steppers continue moving smoothly during all these tasks!
    }
    
    unsigned long total_time = millis() - start_time;
    Serial.print("  Completed "); Serial.print(sensor_readings);
    Serial.print(" sensor readings in "); Serial.print(total_time);
    Serial.println("ms while steppers moved smoothly");
}

// Additional utility functions
void printStepperStatus() {
    Serial.println("Stepper Status:");
    Serial.print("  S1: pos="); Serial.print(stepper1.currentPosition());
    Serial.print(" target="); Serial.print(stepper1.targetPosition());
    Serial.print(" speed="); Serial.println(stepper1.speed());
    
    Serial.print("  S2: pos="); Serial.print(stepper2.currentPosition());
    Serial.print(" target="); Serial.print(stepper2.targetPosition());
    Serial.print(" speed="); Serial.println(stepper2.speed());
    
    Serial.print("  S3: pos="); Serial.print(stepper3.currentPosition());
    Serial.print(" target="); Serial.print(stepper3.targetPosition());
    Serial.print(" speed="); Serial.println(stepper3.speed());
    
    Serial.print("  S4: pos="); Serial.print(stepper4.currentPosition());
    Serial.print(" target="); Serial.print(stepper4.targetPosition());
    Serial.print(" speed="); Serial.println(stepper4.speed());
}

void emergencyStop() {
    Serial.println("EMERGENCY STOP!");
    stepper1.stop();
    stepper2.stop();
    stepper3.stop();
    stepper4.stop();
    AccelInterruptStepper::waitForCompletion();
    Serial.println("All steppers stopped");
}