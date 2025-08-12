# ARM Coordination Troubleshooting Guide

## 🚨 Common Issues & Solutions

This guide covers troubleshooting for ARM coordination problems in the Palletizer system, specifically addressing the blocking delay issue and related coordination problems.

---

## 🔍 Problem Identification Matrix

### Symptom Checklist
Use this checklist to identify the exact problem:

```
❌ ARM2 waits for ARM1 to complete entire sequence
❌ System freezes for 500ms when ARM exits sensor
❌ Only one ARM works at a time (sequential operation)
❌ Low throughput (~1 product per 8-10 seconds)
❌ ARM utilization appears to be ~50%
❌ Serial monitor shows "ARM left center - delay" followed by silence
```

If **3 or more symptoms** match → **Blocking Delay Problem** (follow Section 1)

---

## 🛠️ Section 1: Blocking Delay Problem

### 1.1 Quick Diagnosis
**Test:** Monitor serial output when ARM exits sensor3
**Expected (BROKEN):** 
```
ARM left center - delay
[500ms silence]
Sent HOME to ARM2: ...
```

**Expected (FIXED):**
```
ARM left center - starting non-blocking delay
ARM1 State: PICKING -> IDLE
Leave center delay completed - ARM dispatch now allowed
Sent HOME to ARM2: ...
```

### 1.2 Root Cause
**File:** `PalletizerCentralStateMachine.ino`
**Function:** `handleSystemLogicStateMachine()`
**Issue:** `delay(LEAVE_CENTER_DELAY)` blocks main loop

### 1.3 Immediate Fix
**Apply the fix from `ARM_COORDINATION_FIX.md`:**
1. Add non-blocking timer variables
2. Replace blocking delay with timer logic
3. Upload and test

### 1.4 Verification Steps
1. **Serial Output Check:**
   ```
   ✅ "starting non-blocking delay" appears
   ✅ "delay completed" appears after 500ms
   ✅ No long silences in serial output
   ```

2. **Timing Test:**
   ```cpp
   // Add temporary debug code:
   unsigned long test_start = millis();
   // ... ARM coordination logic ...
   Serial.print("Loop execution time: ");
   Serial.println(millis() - test_start);
   // Should be < 50ms, not 500ms+
   ```

3. **ARM Behavior:**
   - ARM1 exits sensor → ARM2 starts moving within 500ms
   - Both ARMs can work simultaneously

---

## 🔧 Section 2: State Machine Issues

### 2.1 ARM Stuck in Wrong State

**Symptoms:**
- ARM stays in `ARM_PICKING` indefinitely
- ARM never transitions to `ARM_IDLE`
- State machine appears frozen

**Diagnosis Commands:**
```cpp
// Add to loop() for debugging:
if (debug_mode) {
  Serial.print("ARM1 State: "); Serial.println(getStateString(arm1_sm.state));
  Serial.print("ARM2 State: "); Serial.println(getStateString(arm2_sm.state));
  Serial.print("ARM1 Busy: "); Serial.println(arm1_sm.is_busy);
  Serial.print("ARM2 Busy: "); Serial.println(arm2_sm.is_busy);
}
```

**Common Causes:**
1. **Hardware Issue:** ARM_BUSY pin stuck HIGH
2. **State Logic Bug:** Transition conditions not met
3. **Timeout Not Working:** State timeouts disabled

**Solutions:**
1. **Check Hardware:**
   ```cpp
   Serial.print("ARM1 Hardware Pin: "); Serial.println(digitalRead(ARM1_PIN));
   Serial.print("ARM2 Hardware Pin: "); Serial.println(digitalRead(ARM2_PIN));
   ```

2. **Force State Reset:**
   ```cpp
   // Emergency state reset (use via USB serial):
   if (command == "RESET_ARM1") {
     changeArmState(&arm1_sm, ARM_IDLE);
     arm1_sm.is_busy = false;
   }
   ```

3. **Check Timeout Logic:**
   ```cpp
   // Verify timeout constants:
   Serial.print("Move timeout: "); Serial.println(arm1_sm.MOVE_TIMEOUT);
   Serial.print("Pick timeout: "); Serial.println(arm1_sm.PICK_TIMEOUT);
   ```

### 2.2 ARM State Synchronization Issues

**Problem:** ARMs interfering with each other's state transitions

**Debug Code:**
```cpp
void debugArmStates() {
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    Serial.println("=== ARM STATES DEBUG ===");
    Serial.print("ARM1: "); Serial.print(getStateString(arm1_sm.state));
    Serial.print(" (pos "); Serial.print(arm1_sm.current_pos); Serial.println(")");
    Serial.print("ARM2: "); Serial.print(getStateString(arm2_sm.state));
    Serial.print(" (pos "); Serial.print(arm2_sm.current_pos); Serial.println(")");
    Serial.print("arm_in_center: "); Serial.println(arm_in_center);
    Serial.print("sensor3_state: "); Serial.println(sensor3_state);
    Serial.println("========================");
    lastPrint = millis();
  }
}
```

---

## 🔌 Section 3: Communication Issues

### 3.1 RS485 Communication Problems

**Symptoms:**
- ARMs don't respond to commands
- Checksum errors in serial output
- Commands sent but no ARM movement

**Diagnosis:**
1. **Check Wiring:**
   - RS485 RO/DI pins correct
   - Common ground between devices
   - Proper termination resistors

2. **Monitor Communication:**
   ```cpp
   // Add in sendRS485Command():
   Serial.print("Sending: "); Serial.println(fullCommand);
   Serial.print("Checksum: "); Serial.println(checksum, HEX);
   ```

3. **Test Individual ARMs:**
   ```cpp
   // Send direct commands via USB:
   ARML#HOME(1000,1000,1000,1000,1000)*XX
   ARMR#HOME(1000,1000,1000,1000,1000)*XX
   ```

**Solutions:**
1. **Verify Checksum Calculation:**
   ```cpp
   // Test checksum manually:
   String test = "ARML#HOME(1000,1000,1000,1000,1000)";
   uint8_t checksum = calculateXORChecksum(test.c_str(), test.length());
   Serial.print("Test checksum: "); Serial.println(checksum, HEX);
   ```

2. **Check Baud Rate Consistency:**
   - All devices should use 9600 baud
   - Verify crystal oscillator accuracy

3. **Test Loopback:**
   - Connect TX to RX on same device
   - Send command and verify reception

### 3.2 Driver Communication Issues

**Problem:** ARM Control → Driver communication failing

**Check Driver ID Detection:**
```cpp
// In PalletizerArmDriver, add debug:
void debugDriverID() {
  Serial.print("Strap readings: ");
  Serial.print(digitalRead(STRAP_1_PIN)); Serial.print(" ");
  Serial.print(digitalRead(STRAP_2_PIN)); Serial.print(" ");
  Serial.print(digitalRead(STRAP_3_PIN)); Serial.print(" ");
  Serial.print("Driver ID: "); Serial.println(driverID);
}
```

**Verify Command Parsing:**
```cpp
// Monitor incoming commands:
void debugCommandParsing(String command) {
  Serial.print("Raw command: "); Serial.println(command);
  Serial.print("First char: "); Serial.println(command.charAt(0));
  Serial.print("My ID: "); Serial.println(driverID);
  Serial.print("Match: "); Serial.println(command.charAt(0) == driverID);
}
```

---

## 📊 Section 4: Performance Issues

### 4.1 Slow ARM Movement

**Symptoms:**
- ARMs move very slowly
- Long delays between movements
- Timeouts occurring frequently

**Check Speed Settings:**
```cpp
// In PalletizerArmDriver:
Serial.print("Max Speed: "); Serial.println(MOVE_MAX_SPEED);
Serial.print("Acceleration: "); Serial.println(MOVE_ACCELERATION);
Serial.print("A1-A2 Connection: "); // Check speed selection logic
```

**Speed Configuration Debug:**
```cpp
// Test speed selection:
void debugSpeedSelection() {
  pinMode(SPEED_SELECT_PIN_1, OUTPUT);
  digitalWrite(SPEED_SELECT_PIN_1, LOW);
  pinMode(SPEED_SELECT_PIN_2, INPUT_PULLUP);
  delay(10);
  bool a2_low = digitalRead(SPEED_SELECT_PIN_2);
  
  digitalWrite(SPEED_SELECT_PIN_1, HIGH);
  delay(10);
  bool a2_high = digitalRead(SPEED_SELECT_PIN_2);
  
  Serial.print("A2 when A1 low: "); Serial.println(a2_low);
  Serial.print("A2 when A1 high: "); Serial.println(a2_high);
  Serial.print("Connected: "); Serial.println(a2_low == LOW && a2_high == HIGH);
}
```

### 4.2 Coordination Timing Issues

**Problem:** ARMs not coordinating properly despite fix

**Advanced Timing Debug:**
```cpp
void debugCoordinationTiming() {
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 500) {
    Serial.println("=== COORDINATION DEBUG ===");
    Serial.print("leave_center_delay_active: "); Serial.println(leave_center_delay_active);
    if (leave_center_delay_active) {
      Serial.print("Delay remaining: "); 
      Serial.println(LEAVE_CENTER_DELAY - (millis() - leave_center_timer));
    }
    Serial.print("sensor3_state: "); Serial.println(sensor3_state);
    Serial.print("arm_in_center: "); Serial.println(arm_in_center);
    lastDebug = millis();
  }
}
```

---

## 🚑 Emergency Procedures

### Emergency Reset Command
Add this to USB command processing:
```cpp
if (command == "EMERGENCY_RESET") {
  // Reset all ARM states
  changeArmState(&arm1_sm, ARM_IDLE);
  changeArmState(&arm2_sm, ARM_IDLE);
  arm1_sm.is_busy = false;
  arm2_sm.is_busy = false;
  arm_in_center = 0;
  leave_center_delay_active = false;
  Serial.println("EMERGENCY RESET COMPLETE");
}
```

### System Health Check
```cpp
void systemHealthCheck() {
  Serial.println("=== SYSTEM HEALTH CHECK ===");
  Serial.print("Free RAM: "); Serial.println(freeMemory()); // If available
  Serial.print("Loop frequency: "); // Calculate loop Hz
  Serial.print("Sensor readings: ");
  Serial.print(sensor1_state); Serial.print(" ");
  Serial.print(sensor2_state); Serial.print(" ");
  Serial.println(sensor3_state);
  Serial.print("ARM busy pins: ");
  Serial.print(digitalRead(ARM1_PIN)); Serial.print(" ");
  Serial.println(digitalRead(ARM2_PIN));
}
```

### Force Manual Operation
```cpp
// USB commands for manual control:
if (command.startsWith("MANUAL_ARM1_")) {
  String action = command.substring(12);
  executeCommand(("ARML#" + action).c_str());
}
if (command.startsWith("MANUAL_ARM2_")) {
  String action = command.substring(12);
  executeCommand(("ARMR#" + action).c_str());
}
```

---

## 📋 Diagnostic Checklist

### Level 1: Basic Function
- [ ] System compiles and uploads successfully
- [ ] Serial monitor shows system startup messages
- [ ] Sensors reading correctly (check with debug mode)
- [ ] ARM busy pins responding to movement

### Level 2: Communication
- [ ] RS485 commands being sent (check serial output)
- [ ] ARMs receiving and acknowledging commands
- [ ] Checksums calculating correctly
- [ ] Driver IDs detected properly

### Level 3: Coordination
- [ ] ARM state transitions working correctly
- [ ] No blocking delays in main loop
- [ ] Both ARMs can operate concurrently
- [ ] Sensor3 transitions handled properly

### Level 4: Performance
- [ ] Throughput meets expectations (4-5 sec/product)
- [ ] ARM utilization >80%
- [ ] No unnecessary delays or pauses
- [ ] Error recovery working properly

---

## 📞 When to Seek Help

### Contact Support If:
1. **Hardware Issues:** ARM motors not responding to any commands
2. **Electrical Problems:** Smoke, burning smell, or component damage
3. **Safety Concerns:** ARM movement appears dangerous or uncontrolled
4. **Data Corruption:** EEPROM parameters corrupted repeatedly

### Self-Diagnosis First:
1. **Software Issues:** State machine logic problems
2. **Timing Issues:** Coordination delays or sequencing
3. **Communication Glitches:** Intermittent command failures
4. **Parameter Tuning:** Speed or position adjustments

---

## 📝 Maintenance Log Template

```
Date: ___________
Issue Description: _________________________________
Symptoms Observed: ________________________________
Diagnostic Steps: _________________________________
Solution Applied: _________________________________
Test Results: ____________________________________
Follow-up Required: ______________________________
```

---

**Created:** 2025-08-12  
**Status:** Comprehensive Troubleshooting Guide  
**Use Case:** ARM coordination problems and general system issues  
**Difficulty:** Beginner to Advanced