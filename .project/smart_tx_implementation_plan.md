# Smart Transmission Implementation Plan

## Overview
Implement intelligent variable_set frame transmission that adapts transmission rate based on value changes. Values that change frequently are transmitted quickly, while stable values are transmitted less frequently to reduce CAN bus load.

## Requirements Summary
1. **Change Detection:** Track last transmitted value for each channel
2. **Adaptive Timing:** Fast transmission for changed values, slow for unchanged
3. **Independent Schedules:** Input reading and transmission are decoupled
4. **State Machine:** Simple per-channel state tracking

## Design Decisions

### Transmission Intervals
- **Fast Interval (Changed):** 25ms - Transmit immediately when value changes
- **Slow Interval (Unchanged):** 500ms - Transmit periodically even if unchanged (heartbeat)
- **Read Interval:** 25ms - Continue reading inputs at current rate

### Change Detection
- **Analog Inputs:** Threshold-based (e.g., ±2 ADC counts) to avoid noise-triggered transmissions
- **Digital Inputs:** Exact match (bitfield comparison)
- **VSS:** Threshold-based (e.g., ±0.1 PPS) to handle minor fluctuations

### State Machine
Each channel has two states:
- **CHANGED:** Value differs from last transmitted, transmit at fast interval
- **STABLE:** Value matches last transmitted, transmit at slow interval

## Data Structures

### Channel State Structure
```cpp
struct TxChannelState {
    float lastTransmittedValue;    // Last value sent over CAN
    unsigned long lastTxTime;       // Timestamp of last transmission (ms)
    bool hasChanged;                // True if value changed since last TX
    uint8_t state;                  // State machine state (CHANGED/STABLE)
};

// State machine states
#define TX_STATE_CHANGED  0  // Value changed, transmit quickly
#define TX_STATE_STABLE   1  // Value stable, transmit slowly
```

### State Arrays
```cpp
// Analog channels (16)
TxChannelState analogTxState[16];

// Digital channel (1 bitfield)
TxChannelState digitalTxState;

// VSS channels (4)
TxChannelState vssTxState[4];
```

## Implementation Steps

### Step 1: Add Configuration Defines
```cpp
// Transmission intervals
#define TX_INTERVAL_FAST_MS    25   // Fast transmission for changed values
#define TX_INTERVAL_SLOW_MS    500  // Slow transmission for stable values
#define TX_READ_INTERVAL_MS    25   // Input reading interval (unchanged)

// Change detection thresholds
#define TX_ANALOG_THRESHOLD    2.0f    // ADC counts (avoid noise)
#define TX_VSS_THRESHOLD       0.1f    // Pulses per second
// Digital: exact match (no threshold needed)
```

### Step 2: Initialize State Structures
In `setup()`:
- Initialize all `lastTransmittedValue` to 0.0f
- Initialize all `lastTxTime` to 0
- Initialize all `hasChanged` to `true` (force initial transmission)
- Initialize all `state` to `TX_STATE_CHANGED`

### Step 3: Refactor Input Reading
Separate input reading from transmission:
```cpp
void readAnalogInputs(float* values) {
    for (uint8_t i = 0; i < 16; ++i) {
        values[i] = (float)analogRead(A0 + i);
    }
}

void readDigitalInputs(uint16_t* bits) {
    *bits = 0;
    for (uint8_t pin = 22; pin <= 37; ++pin) {
        uint8_t bitIndex = pin - 22;
        if (digitalRead(pin) == LOW) {
            *bits |= (1u << bitIndex);
        }
    }
}

void readVSSInputs(float* values) {
    // VSS values already calculated in calculateVSSRates()
    for (uint8_t i = 0; i < 4; ++i) {
        values[i] = vssChannels[i].pulsesPerSecond;
    }
}
```

### Step 4: Implement Change Detection
```cpp
bool hasAnalogChanged(uint8_t channel, float newValue) {
    float diff = newValue - analogTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff; // abs
    return (diff >= TX_ANALOG_THRESHOLD);
}

bool hasDigitalChanged(uint16_t newBits) {
    return (newBits != (uint16_t)digitalTxState.lastTransmittedValue);
}

bool hasVSSChanged(uint8_t channel, float newValue) {
    float diff = newValue - vssTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff; // abs
    return (diff >= TX_VSS_THRESHOLD);
}
```

### Step 5: Implement State Machine Update
```cpp
void updateTxState(TxChannelState* state, bool changed, unsigned long nowMs) {
    if (changed) {
        state->hasChanged = true;
        state->state = TX_STATE_CHANGED;
    } else {
        state->hasChanged = false;
        // Transition to STABLE only after fast transmission
        if (state->state == TX_STATE_CHANGED && 
            (nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS) {
            state->state = TX_STATE_STABLE;
        }
    }
}
```

### Step 6: Implement Smart Transmission Logic
```cpp
bool shouldTransmit(TxChannelState* state, unsigned long nowMs) {
    if (state->hasChanged) {
        // Changed: transmit if fast interval elapsed
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS);
    } else {
        // Stable: transmit if slow interval elapsed
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_SLOW_MS);
    }
}

void transmitIfNeeded(TxChannelState* state, int32_t varHash, float value, unsigned long nowMs) {
    if (shouldTransmit(state, nowMs)) {
        sendVariableSetFrame(varHash, value);
        state->lastTransmittedValue = value;
        state->lastTxTime = nowMs;
        state->hasChanged = false;
        if (state->state == TX_STATE_CHANGED) {
            state->state = TX_STATE_STABLE;
        }
    }
}
```

### Step 7: Refactor Main Loop
```cpp
void loop() {
    // CAN RX handling (unchanged)
    if(CAN_MSGAVAIL == CAN.checkReceive()) {
        CAN.readMsgBuf(&len, buf);
        unsigned long canId = CAN.getCanId();
    }

    // VSS rate calculation (unchanged)
    calculateVSSRates();
    
    unsigned long nowMs = millis();
    
    // Read inputs at fixed interval (independent of transmission)
    static unsigned long lastReadMs = 0;
    if (nowMs - lastReadMs >= TX_READ_INTERVAL_MS) {
        lastReadMs = nowMs;
        
        // Read all inputs
        float analogValues[16];
        readAnalogInputs(analogValues);
        
        uint16_t digitalBits;
        readDigitalInputs(&digitalBits);
        
        float vssValues[4];
        readVSSInputs(vssValues);
        
        // Update change detection and state machines
        for (uint8_t i = 0; i < 16; ++i) {
            bool changed = hasAnalogChanged(i, analogValues[i]);
            updateTxState(&analogTxState[i], changed, nowMs);
        }
        
        bool digitalChanged = hasDigitalChanged(digitalBits);
        updateTxState(&digitalTxState, digitalChanged, nowMs);
        
        for (uint8_t i = 0; i < 4; ++i) {
            bool changed = hasVSSChanged(i, vssValues[i]);
            updateTxState(&vssTxState[i], changed, nowMs);
        }
    }
    
    // Transmit based on smart scheduling (runs every loop iteration)
    for (uint8_t i = 0; i < 16; ++i) {
        // Note: Need to store current values in static arrays
        transmitIfNeeded(&analogTxState[i], VAR_HASH_ANALOG[i], 
                        currentAnalogValues[i], nowMs);
        delayMicroseconds(200);
    }
    
    transmitIfNeeded(&digitalTxState, VAR_HASH_D22_D37, 
                    (float)currentDigitalBits, nowMs);
    delayMicroseconds(200);
    
    const int32_t vssHashes[4] = {
        VAR_HASH_VSS_FRONT_LEFT, VAR_HASH_VSS_FRONT_RIGHT,
        VAR_HASH_VSS_REAR_LEFT, VAR_HASH_VSS_REAR_RIGHT
    };
    
    for (uint8_t i = 0; i < 4; ++i) {
        transmitIfNeeded(&vssTxState[i], vssHashes[i], 
                        currentVssValues[i], nowMs);
        delayMicroseconds(200);
    }
}
```

## Refinement: Store Current Values

Since transmission happens independently of reading, we need to store current values:

```cpp
// Current input values (updated during read cycle)
static float currentAnalogValues[16] = {0};
static uint16_t currentDigitalBits = 0;
static float currentVssValues[4] = {0};
```

Update these during the read cycle, use them during transmission.

## Edge Cases and Considerations

### Initial Transmission
- All channels start in `TX_STATE_CHANGED` with `hasChanged = true`
- First transmission happens immediately (within fast interval)
- Ensures ECU receives initial state

### millis() Overflow
- Use unsigned subtraction: `(nowMs - lastTxTime)` handles overflow correctly
- Same pattern as existing VSS rate calculation

### Threshold Selection
- **Analog:** 2 ADC counts = ~10mV (reasonable noise threshold)
- **VSS:** 0.1 PPS (very small, catches meaningful changes)
- **Digital:** Exact match (no threshold needed for bitfield)

### CAN Bus Load
- **Worst Case (All Changed):** 21 channels × 40 Hz = 840 frames/sec (within limits)
- **Typical Case (Mixed):** ~10 changed × 40 Hz + 11 stable × 2 Hz = ~420 frames/sec
- **Best Case (All Stable):** 21 channels × 2 Hz = 42 frames/sec

### Memory Usage
- **State Structures:** ~21 × 12 bytes = ~252 bytes (acceptable)
- **Current Values:** 16 × 4 + 2 + 4 × 4 = 82 bytes
- **Total:** ~334 bytes (well within SRAM limits)

## Testing Strategy

1. **Static Values:** Verify slow transmission (500ms interval)
2. **Changing Values:** Verify fast transmission (25ms interval)
3. **Mixed Scenario:** Some channels changing, some stable
4. **Threshold Testing:** Analog noise should not trigger fast transmission
5. **Initial State:** All channels transmit on startup
6. **CAN Bus Load:** Monitor with CAN analyzer to verify frame rates

## Code Organization

### Proposed Structure
```
// Configuration
#define TX_INTERVAL_FAST_MS ...
#define TX_INTERVAL_SLOW_MS ...
#define TX_ANALOG_THRESHOLD ...

// Data structures
struct TxChannelState { ... };
TxChannelState analogTxState[16];
TxChannelState digitalTxState;
TxChannelState vssTxState[4];

// Current values storage
static float currentAnalogValues[16];
static uint16_t currentDigitalBits;
static float currentVssValues[4];

// Functions
void readAnalogInputs(float* values);
void readDigitalInputs(uint16_t* bits);
void readVSSInputs(float* values);
bool hasAnalogChanged(uint8_t channel, float newValue);
bool hasDigitalChanged(uint16_t newBits);
bool hasVSSChanged(uint8_t channel, float newValue);
void updateTxState(TxChannelState* state, bool changed, unsigned long nowMs);
bool shouldTransmit(TxChannelState* state, unsigned long nowMs);
void transmitIfNeeded(TxChannelState* state, int32_t varHash, float value, unsigned long nowMs);
```

## Migration Path

1. Add data structures and defines
2. Implement helper functions (change detection, state machine)
3. Refactor input reading into separate functions
4. Update main loop to use smart transmission
5. Test and tune thresholds/intervals
6. Remove old `TRANSMIT_INTERVAL_MS` define (replaced by smart intervals)

## Open Questions

1. **Threshold Tuning:** Should thresholds be configurable or hard-coded?
   - **Recommendation:** Start with hard-coded, make configurable if needed

2. **Slow Interval:** Is 500ms appropriate for heartbeat?
   - **Recommendation:** Start with 500ms, adjust based on ECU requirements

3. **Fast Interval:** Is 25ms fast enough for responsive updates?
   - **Recommendation:** Start with 25ms (matches current rate), can reduce if needed

4. **Digital Input Debouncing:** Should we add debouncing before change detection?
   - **Recommendation:** Not needed initially; bitfield changes are discrete

5. **VSS Special Handling:** VSS already has rate calculation interval (200ms). Should transmission align?
   - **Recommendation:** Keep independent; smart TX handles VSS like other channels

## Success Criteria

- ✅ Changed values transmit at fast interval (25ms)
- ✅ Unchanged values transmit at slow interval (500ms)
- ✅ Input reading continues at 25ms regardless of transmission rate
- ✅ CAN bus load reduced when values are stable
- ✅ No missed transmissions on value changes
- ✅ Initial state transmitted on startup
- ✅ Code remains maintainable and readable

