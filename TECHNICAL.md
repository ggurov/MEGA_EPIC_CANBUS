# Technical Documentation

Technical details, architecture, and protocol implementation for MEGA_EPIC_CANBUS.

## Architecture Overview

### System Architecture

```
[epicEFI ECU] <--CAN Bus--> [Arduino Mega2560 + MCP_CAN Shield] <--I/O--> [Sensors/Actuators]
```

### Firmware Architecture

**Single-File Design:**
- `mega_epic_canbus.ino` - Complete firmware in single Arduino sketch
- Organized into logical sections with clear separation
- Optional features via conditional compilation (#ifdef)

**Main Components:**
1. **Protocol Layer** - EPIC_CAN_BUS protocol implementation
2. **I/O Layer** - Analog, digital, PWM I/O handling
3. **CAN Layer** - MCP_CAN library wrapper and frame handling
4. **Error Handling** - Watchdog, safe mode, retry logic
5. **Performance** - Change detection, heartbeat mechanism
6. **Testing** - Self-tests, diagnostics, statistics

## Protocol Implementation

### EPIC Over CANbus Protocol

**CAN ID Base Offsets:**
- `0x700 + ECU_CAN_ID` - Variable Request (Arduino → ECU)
- `0x720 + ECU_CAN_ID` - Variable Response (ECU → Arduino)
- `0x740 + ECU_CAN_ID` - Function Request (Arduino → ECU)
- `0x760 + ECU_CAN_ID` - Function Response (ECU → Arduino)
- `0x780 + ECU_CAN_ID` - Variable Set (Arduino → ECU)

**Data Format:**
- **Byte Order:** Big-endian (network byte order)
- **Float32:** IEEE 754 single-precision (4 bytes)
- **Int32:** Signed 32-bit integer (4 bytes)
- **Uint16:** Unsigned 16-bit integer (2 bytes)

### Frame Formats

#### Variable Request (0x700 + ECU_CAN_ID)

**Purpose:** Request variable value from ECU

**Frame Format:**
```
[Variable Hash (int32, 4 bytes)]
```

**DLC:** 4 bytes

**Example:**
```cpp
sendVariableRequest(VAR_HASH_D35_D49);
```

#### Variable Response (0x720 + ECU_CAN_ID)

**Purpose:** ECU responds with variable value

**Frame Format:**
```
[Variable Hash (int32, 4 bytes)] [Value (float32, 4 bytes)]
```

**DLC:** 8 bytes

**Processing:**
- Extract variable hash (bytes 0-3)
- Extract value (bytes 4-7)
- Match hash to I/O channel
- Apply value to output

#### Variable Set (0x780 + ECU_CAN_ID)

**Purpose:** Set variable value in ECU (sensor data transmission)

**Frame Format:**
```
[Variable Hash (int32, 4 bytes)] [Value (float32, 4 bytes)]
```

**DLC:** 8 bytes

**Example:**
```cpp
sendVariableSetFrame(VAR_HASH_ANALOG[0], 512.0);  // ADC value
```

#### Function Request (0x740 + ECU_CAN_ID)

**Purpose:** Call function on ECU with arguments

**Frame Format (0 args):**
```
[Function ID (uint16, 2 bytes)] [Arg1 = 0.0 (float32, 4 bytes)]
```

**Frame Format (1 arg):**
```
[Function ID (uint16, 2 bytes)] [Arg1 (float32, 4 bytes)]
```

**Frame Format (2 args):**
```
[Function ID (uint16, 2 bytes)] [Arg1 (float32, 4 bytes)] [Arg2 (int16, 2 bytes)]
```

**DLC:** 6 or 8 bytes

**Example:**
```cpp
sendFunctionRequest1(FUNC_ID_setFuelAdd, 10.0);  // 10% fuel adjustment
```

#### Function Response (0x760 + ECU_CAN_ID)

**Purpose:** ECU responds with function return value

**Frame Format:**
```
[Function ID (uint16, 2 bytes)] [Reserved (uint16, 2 bytes)] [Return Value (float32, 4 bytes)]
```

**DLC:** 8 bytes

**Return Value:**
- Bytes 4-7 contain return value (float32)
- 0.0 typically indicates error or no return value

### Data Conversion

**Big-Endian Conversion:**

All multi-byte values are transmitted in big-endian (network byte order):

```cpp
// Write int32 big-endian
void writeInt32BigEndian(int32_t value, unsigned char* out) {
    out[0] = (unsigned char)((value >> 24) & 0xFF);
    out[1] = (unsigned char)((value >> 16) & 0xFF);
    out[2] = (unsigned char)((value >> 8) & 0xFF);
    out[3] = (unsigned char)(value & 0xFF);
}

// Read float32 big-endian
float readFloat32BigEndian(unsigned char* in) {
    union { float f; uint32_t i; } converter;
    converter.i = ((uint32_t)in[0] << 24) |
                  ((uint32_t)in[1] << 16) |
                  ((uint32_t)in[2] << 8) |
                  ((uint32_t)in[3]);
    return converter.f;
}
```

## I/O Implementation

### Analog Inputs (A0-A15)

**Hardware:**
- 10-bit ADC (0-1023 counts)
- 0-5V input range
- Internal pullup enabled (affects floating inputs)

**Software:**
- Read via `analogRead(A0 + channel)`
- Convert to float32 for transmission
- Optional calibration (offset + gain)

**Change Detection:**
- Transmit only if change >= threshold
- Heartbeat mechanism (send all values periodically)

**Transmission:**
- One variable_set frame per channel
- Transmitted every TRANSMIT_INTERVAL_MS (if changed or heartbeat)

### Digital Inputs (D20-D34)

**Hardware:**
- 15 pins (D20-D34)
- Internal pullup enabled
- Logic: LOW (grounded) = 1 (active), HIGH (pullup) = 0 (inactive)

**Software:**
- Pack into 15-bit bitfield
- Bit 0 = D20, Bit 14 = D34
- Transmit as float32 (cast to uint16_t on ECU side)

**Change Detection:**
- Transmit only on state change (by default)
- Reduces CAN traffic for buttons/switches

### Digital Outputs (D35-D49)

**Hardware:**
- 15 pins (D35-D49)
- Output: HIGH or LOW
- Safe state: LOW (on communication loss)

**Software:**
- Poll ECU via variable_request
- Receive 15-bit bitfield in variable_response
- Unpack bitfield and apply to pins

**Polling:**
- Request every DIGITAL_OUTPUT_POLL_INTERVAL_MS (50ms)
- Update outputs only if communication healthy

### PWM Outputs (D2-D8, D10-D13, D44-D46)

**Hardware:**
- 14 PWM-capable pins
- 8-bit resolution (0-255)
- Frequency: ~490Hz (D2-D13), ~980Hz (D44-D46)

**Software:**
- Poll ECU via variable_request (round-robin)
- Receive duty cycle (0-100% float32)
- Convert to 0-255 range for `analogWrite()`

**Polling:**
- One channel per PWM_POLL_INTERVAL_MS (100ms)
- Round-robin through all 14 channels
- ~1.4 seconds to poll all channels

## Error Handling

### ECU Communication Watchdog

**Purpose:** Detect ECU communication loss and enter safe mode

**Implementation:**
- Track `lastEcuResponseMs` timestamp
- Check timeout every loop iteration
- Enter safe mode if timeout exceeded (ECU_COMM_TIMEOUT_MS)

**Safe Mode:**
- All digital outputs → LOW
- All PWM outputs → 0% duty cycle
- Prevents unsafe output states

### CAN TX Retry Logic

**Purpose:** Improve reliability of critical transmissions

**Implementation:**
- Retry up to CAN_TX_RETRY_COUNT times on failure
- Delay CAN_TX_RETRY_DELAY_MS between retries
- Used for critical frames (variable_set with retry)

### Frame Validation

**Checks:**
- Frame length (must be 1-8 bytes)
- CAN ID range (must be EPIC protocol range)
- ECU ID match (must match ECU_CAN_ID)

**Error Handling:**
- Increment error counter
- Log error message (if enabled)
- Discard invalid frame

## Performance Optimization

### Change Detection

**Analog Inputs:**
- Track previous value per channel
- Transmit only if change >= threshold
- Reduces CAN traffic by ~80-90%

**Digital Inputs:**
- Track previous bitfield state
- Transmit only on state change
- Reduces CAN traffic for buttons/switches

### Heartbeat Mechanism

**Purpose:** Prevent stale data in ECU

**Implementation:**
- Send all analog values periodically (ANALOG_HEARTBEAT_INTERVAL_MS)
- Ensures ECU receives updates even if values are static
- Prevents timeout issues in ECU

### Polling Optimization

**Digital Outputs:**
- Single variable_request per polling interval
- ECU responds with 15-bit bitfield
- Efficient: 1 request → 15 outputs updated

**PWM Outputs:**
- Round-robin polling (one channel per interval)
- Spreads CAN traffic over time
- Prevents burst traffic

## Memory Usage

**Flash (Program):**
- Core firmware: ~20-25KB
- With all features: ~30-35KB
- Available: 256KB (Mega2560) - plenty of headroom

**SRAM:**
- Static variables: ~200-300 bytes
- Stack: ~500-1000 bytes (typical)
- Available: 8KB (Mega2560) - sufficient

**EEPROM:**
- Magic number: 4 bytes
- ECU_CAN_ID: 1 byte
- ADC calibration: 32 bytes (if enabled)
- Total: ~37 bytes (if all features enabled)

## Timing Characteristics

### Transmission Timing

**Analog/Digital Inputs:**
- Transmit interval: 25ms default
- Rate: ~40 frames/second (with change detection)
- Actual rate: ~5-10 frames/second (most frames filtered)

**Output Polling:**
- Digital outputs: 50ms interval (20 requests/second)
- PWM outputs: 100ms per channel (~0.7 requests/second per channel)

### Latency

**Sensor → ECU:**
- Reading: <1ms
- Change detection: <1ms
- CAN TX: <10ms (worst case with retries)
- **Total:** <25ms (transmit interval)

**ECU → Output:**
- CAN RX polling: 0-100ms (polling interval)
- Frame processing: <1ms
- Output update: <1ms
- **Total:** <100ms worst case (50ms typical for digital, 100ms for PWM)

### Loop Timing

**Main Loop:**
- CAN RX processing: <1ms (per frame)
- I/O reading: <5ms (all channels)
- Change detection: <1ms
- CAN TX: <1ms (per frame)
- **Total loop time:** <10ms typical

## Code Organization

### Section Structure

1. **Includes & Constants** - Libraries, pin definitions, configuration
2. **Variable Hashes** - EPIC variable hash mappings
3. **Static Variables** - State tracking, counters, previous values
4. **Helper Functions** - Big-endian conversion, frame composition
5. **I/O Functions** - Self-test functions, calibration
6. **Protocol Functions** - CAN frame send/receive, parsing
7. **Setup Function** - Initialization
8. **Loop Function** - Main execution loop

### Conditional Compilation

Optional features controlled by preprocessor defines:

```cpp
#if ENABLE_INTERRUPT_COUNTERS
    // Interrupt counter code
#endif

#if ENABLE_ADC_CALIBRATION
    // ADC calibration code
#endif
```

**Benefits:**
- Reduces code size when features disabled
- Easy to enable/disable features
- Clear feature boundaries

## Extension Points

### Adding New I/O Types

1. **Define Pin Array:**
   ```cpp
   const uint8_t NEW_PINS[] = { /* ... */ };
   ```

2. **Define Variable Hashes:**
   ```cpp
   const int32_t VAR_HASH_NEW[] = { /* ... */ };
   ```

3. **Add Polling Logic:**
   - In `loop()`, add periodic request
   - Handle response in `processCanFrame()`

4. **Add Safe Mode:**
   - Set safe state in `enterSafeMode()`

### Adding New Functions

1. **Define Function ID:**
   ```cpp
   #define FUNC_ID_newFunction 100
   ```

2. **Create Wrapper:**
   ```cpp
   static inline bool callNewFunction(float arg1) {
       return sendFunctionRequest1(FUNC_ID_newFunction, arg1);
   }
   ```

3. **Handle Response:**
   - In `processCanFrame()`, check `funcId` in function_response
   - Process return value

## Development Guidelines

### Code Style

- **Functions:** `camelCase` for functions, `UPPER_CASE` for macros
- **Variables:** `camelCase` for static, `camelCase` for local
- **Constants:** `UPPER_CASE` for defines, `camelCase` for const arrays
- **Comments:** Clear, concise, explain "why" not "what"

### Error Handling

- **CAN TX:** Check return value, retry on failure
- **Frame RX:** Validate length, ID range, data format
- **I/O:** Validate ranges before applying
- **Safe Mode:** Always enter on communication loss

### Performance

- **Minimize CAN Traffic:** Use change detection
- **Efficient Polling:** Round-robin, staggered intervals
- **Fast Processing:** Avoid delays in main loop
- **Memory:** Use static arrays, avoid dynamic allocation

## Protocol Reference

See `.project/epic_can_bus_spec.txt` for complete EPIC protocol specification.

## See Also

- `README.md` - User documentation
- `CONFIGURATION.md` - Configuration guide
- `PIN_ASSIGNMENT.md` - Hardware details

