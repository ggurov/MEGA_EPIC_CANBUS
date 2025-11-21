# System Patterns: MEGA_EPIC_CANBUS

## Architecture Overview

### Three-Layer Design
```
┌─────────────────────────────────────┐
│     I/O Layer (Pin Management)      │
│  - Analog reads (A0-A15)            │
│  - Digital reads (D22-D37)          │
│  - Digital writes (D39-D43, D47-D49)│
│  - PWM outputs (D2-D13, D44-D46)    │
│  - VSS interrupts (D18-D21)         │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│  Protocol Layer (EPIC_CAN_BUS)      │
│  - Frame parsing/composition        │
│  - Variable request/response        │
│  - Variable set (fire-and-forget)   │
│  - Function call request/response   │
└──────────────┬──────────────────────┘
               │
┌──────────────▼──────────────────────┐
│   Transport Layer (MCP_CAN/SPI)     │
│  - CAN frame TX/RX via MCP2515      │
│  - SPI communication                │
│  - Hardware interrupt handling      │
└─────────────────────────────────────┘
```

## Module Organization

### Proposed Code Structure
```
mega_epic_canbus.ino       - Main sketch, setup(), loop()
epic_protocol.h/.cpp       - EPIC protocol frame handling
analog_inputs.h/.cpp       - Analog input management
digital_io.h/.cpp          - Digital input/output management
pwm_outputs.h/.cpp         - PWM output management
variable_map.h             - Variable hash to I/O mappings
```

*Note: Current implementation is single-file; modularization pending.*

## Data Flow Patterns

### Analog Input Flow (Mega → ECU)
1. **Sample:** Periodic `analogRead()` on A0-A15 (e.g., 100 Hz)
2. **Convert:** Raw ADC value (0-1023) → float32
3. **Pack:** Create variable_set frame (0x780 + ecuCanId)
   - Bytes 0-3: Variable hash (big-endian int32)
   - Bytes 4-7: Value (big-endian float32)
4. **Transmit:** `CAN.sendMsgBuf()` with DLC=8
5. **No ACK:** Fire-and-forget pattern

### Smart Transmission Pattern (On-change + Heartbeat)
1. **Sample:** Inputs (analog, digital, VSS) read on a fixed schedule (e.g., 25 ms)
2. **Detect Change:** Compare current value against last transmitted value using thresholds (analog/VSS) or exact match (digital bitfield)
3. **State Machine:** Per-channel state (`CHANGED` vs `STABLE`) updated based on detection step
4. **Schedule:** 
   - `CHANGED`: permit TX at fast interval (e.g., 25 ms)
   - `STABLE`: permit TX only at slow heartbeat interval (e.g., 500 ms)
5. **Transmit:** When allowed by state/interval, send variable_set frame and update last transmitted value/timestamp

**Rationale:** Reduces CAN load when values are stable while keeping low latency on changes. Uses unsigned `millis()` subtraction to remain safe across overflow.

### Digital Output Flow (ECU → Mega)
1. **Request:** Mega sends variable_request frame (0x700 + ecuCanId)
   - Bytes 0-3: Variable hash for output states
2. **Receive:** ECU responds on 0x720 + ecuCanId
   - Bytes 0-3: Variable hash (echo)
   - Bytes 4-7: Bitfield as float32
3. **Unpack:** Extract bitfield, map to D35-D49
4. **Apply:** `digitalWrite()` on each pin

### PWM Output Flow (ECU → Mega)
1. **Request:** Mega sends variable_request for PWM parameters
2. **Receive:** ECU responds with duty cycle (0-100%) as float32
3. **Apply:** `analogWrite()` on D2-D13, D44-D46
   - Convert percentage to 0-255 range

### GPS Data Flow (Mega → ECU)
1. **Capture:** GPS module (e.g., GT‑U7 / u‑blox7) sends standard NMEA‑0183 sentences (`GPRMC`, `GPGGA`) over Serial2 at a configurable rate (default 20 Hz at 115200 baud for GT‑U7).
2. **Parse:** `nmea_parser.cpp` maintains a small line buffer, validates NMEA checksums, and parses:
   - `GPRMC`: time, date, latitude, longitude, speed over ground, course, fix status.
   - `GPGGA`: fix quality, satellites used, HDOP (accuracy), altitude, and redundant position.
   - Lat/Lon in DDMM.MMMM / DDDMM.MMMM are converted to signed decimal degrees.
3. **Stage:** Parsed values are stored in a `GPSData` struct (time/date, position, quality, sats, accuracy, altitude, speed, course, fix flag).
4. **Smart TX:** The main loop compares current GPS values against last-sent values with small thresholds and:
   - Packs time/date into 2× `uint32_t` words (HMSD + MYQSAT).
   - Sends HMSD/MYQSAT plus individual float variables (lat, lon, speed, course, altitude, accuracy) as EPIC `variable_set` frames.
   - Uses the same on-change + heartbeat strategy as analog/VSS (fast when changing, slow when stable).

**Rationale:** Treat GPS as another set of analog-like channels with their own smart transmission, while keeping parsing decoupled from CAN logic.

## Design Patterns

### State Machine for Protocol Handling
```
IDLE → RX_FRAME → PARSE_ID → DISPATCH_HANDLER → PROCESS → IDLE
                       ↓
                  UNKNOWN_ID → IDLE
```

### Periodic Task Scheduling
Use `millis()` based non-blocking scheduling:
```cpp
unsigned long lastAnalogRead = 0;
const unsigned long ANALOG_INTERVAL = 10; // 100 Hz

void loop() {
  unsigned long now = millis();
  
  if (now - lastAnalogRead >= ANALOG_INTERVAL) {
    readAnalogInputs();
    lastAnalogRead = now;
  }
  
  // Other periodic tasks...
  processCAN();
}
```

**Rationale:** Avoids `delay()`, keeps CAN RX responsive.

### Variable Hash Mapping
Static compile-time lookup table:
```cpp
struct VariableMapping {
  int32_t hash;        // EPIC variable hash
  uint8_t pin;         // Arduino pin number
  const char* name;    // Human-readable (debug only)
};

const VariableMapping analogInputMap[] = {
  {0x12345678, A0, "analog_input_0"},
  {0x23456789, A1, "analog_input_1"},
  // ...
};
```

**Trade-off:** Compile-time mapping is inflexible but fast and deterministic.

## Concurrency Model

### Single-Threaded Event Loop
- No RTOS, no threading
- All work done in `loop()` or ISR
- Non-blocking operations required

### Interrupt Service Routine (ISR)
MCP2515 INT pin (typically D2 on Mega) can trigger ISR:
```cpp
volatile bool canFrameAvailable = false;

void canISR() {
  canFrameAvailable = true;
}

void setup() {
  attachInterrupt(digitalPinToInterrupt(2), canISR, FALLING);
}

void loop() {
  if (canFrameAvailable) {
    canFrameAvailable = false;
    processCAN();
  }
}
```

**Code Smell Alert:** Current implementation uses polling (`CAN.checkReceive()`), not interrupts. Polling is simpler but higher latency and CPU usage.

## Error Handling Strategy

### CAN Bus Errors
- **TX Failure:** Log to Serial, retry once, then drop frame
- **RX Overflow:** MCP2515 has 2 buffers; read as fast as possible to avoid loss
- **Bus Off:** Detect via MCP2515 error flags, attempt re-initialization

### Protocol Errors
- **Invalid Frame:** Log and discard
- **Unknown Variable Hash:** Return 0.0 (EPIC zero-as-error pattern)
- **Malformed Data:** Validate DLC and contents before processing

### Watchdog Pattern
Monitor last successful CAN RX timestamp:
```cpp
unsigned long lastRxTime = 0;
const unsigned long WATCHDOG_TIMEOUT = 5000; // 5 seconds

void loop() {
  if (millis() - lastRxTime > WATCHDOG_TIMEOUT) {
    // ECU communication lost, enter safe mode
    setSafeOutputs();
  }
}
```

## Performance Considerations

### CAN Frame Budget (500 kbps)
- Theoretical max: ~800 frames/sec (standard 11-bit ID)
- Practical Mega+MCP2515: 400-700 frames/sec
- Reserved budget:
  - 160 frames/sec for 16 analog inputs @ 10 Hz each
  - 10 frames/sec for digital input updates
  - 14 frames/sec for PWM parameter requests
  - ~216-526 frames/sec remaining for ECU→Mega

### Memory Constraints
- ATmega2560: 8 KB SRAM
- Avoid dynamic allocation
- Pre-allocate buffers for CAN frames
- Stack usage for deep call chains

### Timing Constraints
- Analog read: ~100 µs per channel
- SPI transaction: ~10-50 µs depending on operation
- CAN frame RX: ~200-500 µs including SPI overhead
- Target: Process CAN RX within 1 ms to keep buffers clear

## Future Architectural Considerations

### VSS Interrupt Counters
External interrupts on D18-D21 (INT3, INT2, INT1, INT0) for wheel speed sensors:
```cpp
volatile uint32_t vssEdgeCount[4] = {0, 0, 0, 0};

ISR(INT3_vect) { vssEdgeCount[2]++; }  // D18, RearLeft
ISR(INT2_vect) { vssEdgeCount[3]++; }  // D19, RearRight
ISR(INT1_vect) { vssEdgeCount[0]++; }  // D20, FrontLeft
ISR(INT0_vect) { vssEdgeCount[1]++; }  // D21, FrontRight
```
**Note:** VSS pins configured with internal pullups to prevent interrupt storms when floating. VR conditioner signals will override pullup when connected. Calculate pulses per second in main loop, report to ECU via variable_set frames.

### Asynchronous ADC
Use ATmega2560 ADC interrupt mode for true non-blocking analog reads. Requires more complex state machine.

### Configuration Storage
EEPROM storage for:
- ecuCanId selection
- Variable hash mappings (if made configurable)
- Calibration offsets

