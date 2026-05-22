# Centralized LED Management System

## Overview
The blue status LED (D7) is now managed through a centralized system in `device_pinout.cpp` rather than scattered `digitalWrite()` calls throughout the codebase. This provides consistent behavior, automatic timed control, and easy status checking.

## API Functions

### `signalLED(bool state, uint32_t durationMs = 0)`
Control the blue status LED with optional automatic turn-off.

**Parameters:**
- `state`: `true` = ON, `false` = OFF
- `durationMs`: Duration in milliseconds (0 = indefinite)

**Behavior:**
- When `durationMs > 0`: LED automatically turns off after specified duration
- When `durationMs = 0`: LED stays in specified state until changed
- Previous duration timers are cancelled when called again

**Examples:**
```cpp
signalLED(true, 1000);  // Flash for 1 second (counting mode)
signalLED(true);        // Turn on indefinitely (occupancy mode)
signalLED(false);       // Turn off immediately
```

### `signalLEDUpdate()`
Process automatic LED turn-off timers.

**Usage:**
Call this regularly in your main loop or state handlers to enable automatic LED turn-off after duration expires.

**Example:**
```cpp
void handleIdleState() {
  signalLEDUpdate();  // Process any pending LED timers
  // ... rest of idle state logic
}
```

### `signalLEDStatus()`
Check current LED state.

**Returns:** `true` if LED is currently ON, `false` if OFF

**Usage:**
Used to check LED state for occupancy gating or status checks.

**Example:**
```cpp
if (current.get_occupied() && !signalLEDStatus()) {
  signalLED(true);  // Keep LED on while occupied
}
```

## Mode-Specific Behavior

### COUNTING Mode
**Behavior:** Brief flash (1 second) on each count
```cpp
// On sensor detection
signalLED(true, 1000);  // Flash for 1 second
```

**Sleep Gating:** LED status checked before sleep to avoid cutting off visible flash
```cpp
if (signalLEDStatus()) {
  // Defer sleep if LED still on (flash in progress)
  state = IDLE_STATE;
  return;
}
```

### OCCUPANCY Mode
**Behavior:** LED on for entire occupancy duration
```cpp
// When becoming occupied
if (!current.get_occupied()) {
  current.set_occupied(true);
  signalLED(true);  // Turn on indefinitely
}

// When becoming unoccupied
if (timeout_expired) {
  current.set_occupied(false);
  signalLED(false);  // Turn off
}
```

**Maintenance:** LED state continuously maintained in IDLE state
```cpp
void handleIdleState() {
  signalLEDUpdate();  // Handle any timed flashes
  
  if (sysStatus.get_sensorMode() == OCCUPANCY) {
    if (current.get_occupied() && !signalLEDStatus()) {
      signalLED(true);   // Keep on while occupied
    } else if (!current.get_occupied() && signalLEDStatus()) {
      signalLED(false);  // Ensure off when unoccupied
    }
  }
}
```

**Sleep Gating:** In occupancy mode, LED status indicates occupancy state
```cpp
if (signalLEDStatus() && sysStatus.get_sensorMode() == OCCUPANCY) {
  // Defer sleep if occupied (LED on means occupied)
  state = IDLE_STATE;
  return;
}
```

## Implementation Details

### Internal State
Located in `device_pinout.cpp`:
```cpp
static uint32_t ledOffTime = 0;  // Millis when LED should turn off (0 = indefinite)
```

### Timer Management
- `ledOffTime = 0`: No automatic turn-off (indefinite ON or currently OFF)
- `ledOffTime > 0`: Scheduled turn-off at specified millis() timestamp
- Calling `signalLED()` again cancels any pending timer

### Automatic Turn-Off
The `signalLEDUpdate()` function checks if turn-off time has been reached:
```cpp
void signalLEDUpdate() {
  if (ledOffTime > 0 && millis() >= ledOffTime) {
    digitalWrite(BLUE_LED, LOW);
    ledOffTime = 0;
  }
}
```

## Migration from Old System

### Removed Components
1. **Timer ISR**: `countSignalTimerISR()` - no longer needed
2. **Timer object**: `Timer countSignalTimer` - removed from global scope
3. **Scattered digitalWrite calls**: Replaced with centralized `signalLED()`

### Migration Examples

**Before:**
```cpp
digitalWrite(BLUE_LED, HIGH);
if (countSignalTimer.isActive()) {
  countSignalTimer.reset();
} else {
  countSignalTimer.start();
}
```

**After:**
```cpp
signalLED(true, 1000);  // Automatic turn-off after 1 second
```

**Before:**
```cpp
if (digitalRead(BLUE_LED) == HIGH) {
  // Do something
}
```

**After:**
```cpp
if (signalLEDStatus()) {
  // Do something
}
```

## Benefits

1. **Consistent Behavior**: All LED control goes through one system
2. **Simplified Code**: Less boilerplate for timed flashes
3. **Mode Awareness**: Easy to implement mode-specific LED behavior
4. **Status Checking**: Simple API to check LED state for gating logic
5. **Maintainability**: LED behavior changes only require updates in one place
6. **Sleep Safety**: Proper LED state tracking prevents premature sleep

## Files Modified

- `src/device_pinout.h`: Added LED management function declarations
- `src/device_pinout.cpp`: Implemented LED management functions
- `src/state/State_Modes.cpp`: Updated to use `signalLED()` API
- `src/state/State_Idle.cpp`: Added `signalLEDUpdate()` call and status checks
- `src/state/State_Sleep.cpp`: Updated sleep gating and wake LED control
- `src/Generalized-Core-Counter.cpp`: Removed timer and ISR, updated LED calls

## Testing Recommendations

### COUNTING Mode
1. Trigger sensor detection
2. Verify LED flashes for ~1 second
3. Verify LED turns off automatically
4. Verify sleep deferred while LED on

### OCCUPANCY Mode
1. Trigger PIR to become occupied
2. Verify LED turns on and stays on
3. Wait for debounce timeout (setting1 = 60000ms = 60 seconds)
4. Verify LED turns off when unoccupied
5. Verify LED stays on during entire occupancy period
6. Verify device connects and publishes on occupancy changes

### Edge Cases
1. Multiple rapid triggers in COUNTING mode
2. Occupancy timeout while in different states
3. Wake from sleep with LED on (occupancy mode)
4. Sleep gating with LED status check
