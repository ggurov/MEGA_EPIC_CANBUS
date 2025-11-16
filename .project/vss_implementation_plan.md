# VSS Pickup Implementation Plan

## Overview
Integrate Vehicle Speed Sensor (VSS) pickup functionality using interrupt-based edge counting on Arduino Mega2560. Count falling edges from 4 wheel speed sensors, convert to pulses per second, and transmit to ECU via CAN bus using EPIC_CAN_BUS variable_set frames.

## Requirements Summary
- **4 Wheel Speed Sensors:** FrontLeft, FrontRight, RearLeft, RearRight
- **Edge Detection:** Falling edges (for external VR conditioner compatibility)
- **Output Format:** Pulses per second (float32) sent via variable_set frames
- **Variable Hashes:**
  - FrontLeft: -1645222329
  - FrontRight: 1549498074
  - RearLeft: 768443592
  - RearRight: -403905157

## Pin Allocation

### Interrupt-Capable Pins on Mega2560
Available external interrupt pins:
- **D2:** INT4 (reserved for MCP_CAN INT, but can be used if not using interrupt-driven CAN RX)
- **D3:** INT5 (available)
- **D18:** INT3 (available if UART1 not used)
- **D19:** INT2 (available if UART1 not used)
- **D20:** INT1 (currently used for digital button inputs)
- **D21:** INT0 (currently used for digital button inputs and I2C)

### Pin Mapping (Selected for Traction Control Consistency)
**All 4 sensors use external interrupts on a contiguous block of pins:**
- **FrontLeft:** D18 (INT3)
- **FrontRight:** D19 (INT2)
- **RearLeft:** D20 (INT1)
- **RearRight:** D21 (INT0)

**Decision:** Use D18-D21 (INT3, INT2, INT1, INT0) for consistent edge detection and ISR behavior. Critical for traction control where all sensors must have identical timing and precision. MCP_CAN RX remains polling-based (`CAN.checkReceive()`), so D2 is not used for CAN INT in this implementation.

## Implementation Architecture

### Data Structures
```cpp
// VSS sensor configuration (all external interrupts for consistency)
#define VSS_FRONT_LEFT_PIN   18  // INT3
#define VSS_FRONT_RIGHT_PIN  19  // INT2
#define VSS_REAR_LEFT_PIN    20  // INT1
#define VSS_REAR_RIGHT_PIN   21  // INT0

// Variable hashes for VSS sensors
const int32_t VAR_HASH_VSS_FRONT_LEFT  = -1645222329;
const int32_t VAR_HASH_VSS_FRONT_RIGHT = 1549498074;
const int32_t VAR_HASH_VSS_REAR_LEFT   = 768443592;
const int32_t VAR_HASH_VSS_REAR_RIGHT  = -403905157;

// VSS counter structure
struct VSSChannel {
    volatile uint32_t edgeCount;      // Edge counter (ISR increments)
    uint32_t lastCount;               // Last count for rate calculation
    unsigned long lastCalcTime;       // Last calculation timestamp (ms)
    float pulsesPerSecond;             // Calculated rate
};

// VSS channels array
VSSChannel vssChannels[4] = {
    {0, 0, 0, 0.0f},  // FrontLeft
    {0, 0, 0, 0.0f},  // FrontRight
    {0, 0, 0, 0.0f},  // RearLeft
    {0, 0, 0, 0.0f}   // RearRight
};
```

### Interrupt Service Routines (ISRs)
```cpp
// ISR for FrontLeft (D18, INT3)
ISR(INT3_vect) {
    vssChannels[0].edgeCount++;
}

// ISR for FrontRight (D19, INT2)
ISR(INT2_vect) {
    vssChannels[1].edgeCount++;
}

// ISR for RearLeft (D20, INT1)
ISR(INT1_vect) {
    vssChannels[2].edgeCount++;
}

// ISR for RearRight (D21, INT0)
ISR(INT0_vect) {
    vssChannels[3].edgeCount++;
}
```

### Rate Calculation
**Strategy:** Time-based rate calculation in main loop
- Calculate pulses/second every N milliseconds (e.g., 100ms)
- Formula: `pulsesPerSecond = (currentCount - lastCount) / (timeDelta / 1000.0)`
- Handle overflow: Use 32-bit counters, handle rollover
- Minimum time window: 50ms minimum to avoid division by zero and noise

### CAN Transmission
- Transmit VSS values at same interval as other I/O (25ms default)
- Use existing `sendVariableSetFrame()` helper
- Send all 4 channels each transmission cycle
- Format: float32 pulses per second

## Implementation Steps

### Step 1: Add VSS Constants and Data Structures
- Add pin definitions for 4 VSS sensors
- Add variable hash constants
- Add VSSChannel structure and array
- Add calculation interval define (e.g., `VSS_CALC_INTERVAL_MS 100`)

### Step 2: Implement Pin Configuration
In `setup()`:
- Configure D18, D19, D20, D21 as INPUT / INPUT_PULLUP (depending on `VSS_ENABLE_PULLUP`)
- Configure INT3 (D18) for falling edge: `EICRB |= (1<<ISC31) | (0<<ISC30);` (falling edge)
- Configure INT2 (D19) for falling edge: `EICRB |= (1<<ISC21) | (0<<ISC20);` (falling edge)
- Configure INT1 (D20) for falling edge: `EICRA |= (1<<ISC11) | (0<<ISC10);` (falling edge)
- Configure INT0 (D21) for falling edge: `EICRA |= (1<<ISC01) | (0<<ISC00);` (falling edge)
- Enable external interrupts: `EIMSK |= (1<<INT3) | (1<<INT2) | (1<<INT1) | (1<<INT0);`
- Enable global interrupts: `sei()`

### Step 3: Implement ISRs
- Implement INT3_vect for FrontLeft (D18)
- Implement INT2_vect for FrontRight (D19)
- Implement INT1_vect for RearLeft (D20)
- Implement INT0_vect for RearRight (D21)

### Step 4: Implement Rate Calculation Function
```cpp
void calculateVSSRates() {
    unsigned long now = millis();
    static unsigned long lastCalcTime = 0;
    
    if (now - lastCalcTime >= VSS_CALC_INTERVAL_MS) {
        unsigned long timeDelta = now - lastCalcTime;
        float timeDeltaSeconds = timeDelta / 1000.0f;
        
        for (uint8_t i = 0; i < 4; ++i) {
            // Atomic read of volatile counter
            uint32_t currentCount = vssChannels[i].edgeCount;
            uint32_t countDelta = currentCount - vssChannels[i].lastCount;
            
            // Calculate pulses per second
            vssChannels[i].pulsesPerSecond = countDelta / timeDeltaSeconds;
            vssChannels[i].lastCount = currentCount;
        }
        
        lastCalcTime = now;
    }
}
```

### Step 5: Integrate into Main Loop
In `loop()`:
- Call `calculateVSSRates()` periodically
- In transmission block, send all 4 VSS values:
```cpp
// Transmit VSS values
const int32_t vssHashes[4] = {
    VAR_HASH_VSS_FRONT_LEFT,
    VAR_HASH_VSS_FRONT_RIGHT,
    VAR_HASH_VSS_REAR_LEFT,
    VAR_HASH_VSS_REAR_RIGHT
};

for (uint8_t i = 0; i < 4; ++i) {
    sendVariableSetFrame(vssHashes[i], vssChannels[i].pulsesPerSecond);
}
```

## Technical Considerations

### Edge Detection
- **Falling Edge:** Configure interrupts for falling edge (ISCx1=1, ISCx0=0)
- **Rationale:** VR conditioners typically output falling edges when tooth passes sensor
- **Debouncing:** Not needed for VR sensors (clean digital signal from conditioner)

### Counter Overflow
- **32-bit counters:** Use `uint32_t` for edgeCount (max 4.29 billion)
- **Overflow handling:** Subtraction handles rollover correctly (unsigned arithmetic)
- **Rate calculation:** Handles count delta correctly even if counter wraps

### Timing Constraints
- **ISR execution:** Keep ISRs minimal (just increment counter)
- **Rate calculation:** Run in main loop, not ISR
- **Update rate:** Calculate every 100ms, transmit every 25ms (use last calculated value)

### Pin Configuration
- **Pullup configurable:** Internal pullups can be enabled to avoid floating pins when sensors disconnected
- **Input mode:** `INPUT` or `INPUT_PULLUP` depending on configuration
- **Signal level:** Expect 0-5V digital signal from VR conditioner
- **Consistency:** All 4 sensors use external interrupts (INT2-INT5) for identical ISR behavior, timing, and edge detection precision. Critical for traction control where sensor-to-sensor timing differences must be minimized.

## Performance Impact

### CAN Bus Load
- **Additional frames:** 4 frames per transmission cycle (25ms interval = 40Hz)
- **Frame rate:** 4 × 40 = 160 frames/sec for VSS
- **Total load:** Existing ~680 frames/sec (16 analog + 1 digital) + 160 VSS = ~840 frames/sec
- **Within limits:** Still below practical 1000+ frames/sec capacity

### CPU Load
- **ISR overhead:** Minimal (single increment per edge)
- **Calculation overhead:** 4 divisions every 100ms (negligible)
- **Memory:** ~32 bytes for VSS data structures

## Testing Strategy

### Unit Testing
1. **ISR functionality:** Verify counters increment on falling edges
2. **Rate calculation:** Test with known pulse rates (signal generator)
3. **Overflow handling:** Test counter rollover behavior
4. **CAN transmission:** Verify frames sent with correct hashes and values

### Integration Testing
1. **Hardware validation:** Connect VR conditioner outputs, verify signal levels
2. **ECU integration:** Verify ECU receives and processes VSS values
3. **Performance:** Measure actual frame rates and CPU usage
4. **Long-term stability:** Run for extended periods, verify no counter overflow issues

## Code Organization

### File Structure
- Add VSS code to main `.ino` file (keep single-file structure for now)
- Group VSS code in sections:
  - Constants and defines
  - Data structures
  - ISR implementations
  - Setup code
  - Calculation function
  - Transmission code

### Code Comments
- Document pin assignments clearly
- Explain interrupt configuration registers
- Note VR conditioner requirements
- Document calculation timing

## Future Enhancements

### Potential Improvements
1. **Configurable pins:** Allow pin assignment via defines
2. **Rising/falling edge selection:** Make edge type configurable
3. **Filtering:** Add low-pass filter for noisy signals
4. **Diagnostics:** Add counter overflow detection and reporting
5. **Calibration:** Add pulses-per-revolution calibration factor

## Documentation Updates Required

### Memory Bank Updates
- **activeContext.md:** Add VSS implementation to current work
- **progress.md:** Mark VSS module as completed when done
- **techContext.md:** Document VSS pin assignments
- **systemPatterns.md:** Document interrupt-based counter pattern

### User Documentation
- **Pin mapping table:** Document which pins connect to which wheel
- **Wiring diagram:** Show VR conditioner connections
- **Configuration:** Document any configurable parameters

## Implementation Checklist

- [ ] Add VSS constants and data structures
- [ ] Configure interrupt pins in setup()
- [ ] Implement 4 ISRs (3 external + 1 pin change)
- [ ] Implement rate calculation function
- [ ] Integrate calculation into main loop
- [ ] Add VSS transmission to CAN send block
- [ ] Test ISR functionality with signal generator
- [ ] Test rate calculation accuracy
- [ ] Verify CAN frame transmission
- [ ] Test with actual VR conditioners
- [ ] Update Memory Bank documentation
- [ ] Create pin mapping documentation

## Estimated Implementation Time
- **Code implementation:** 2-3 hours
- **Testing and debugging:** 2-3 hours
- **Documentation:** 1 hour
- **Total:** 5-7 hours

