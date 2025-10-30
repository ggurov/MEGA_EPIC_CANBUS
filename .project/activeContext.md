# Active Context: MEGA_EPIC_CANBUS

## Current Status
**Phase:** Initial Development - CAN Communication Scaffold  
**Last Updated:** October 30, 2025

## Current Implementation
Baseline Arduino sketch exists (`mega_epic_canbus.ino`) with:
- MCP_CAN library integration
- CAN initialization at 500 kbps
- Basic receive loop that prints incoming CAN frames to Serial
- SPI CS pin 9 configured for MCP_CAN shield

**Code State:**
- Functional CAN receive example
- No EPIC protocol implementation yet
- No I/O handling implemented
- Serial debug output only

## Immediate Next Steps

### 1. Define ECU CAN ID
- Select ecuCanId (0-15) for this Mega device
- Document decision in code and Memory Bank
- Impacts all CAN ID calculations (0x700+ecuCanId, etc.)

### 2. Implement EPIC Protocol Frame Parsing
- Parse incoming CAN IDs to identify EPIC frame types
- Implement variable request/response handling
- Implement variable set reception
- Implement function call request/response
- Big-endian byte order conversion utilities

### 3. Design Variable Mapping Scheme
**Critical Decision Needed:**
- How to map Mega I/O to epicEFI variable hashes
- Static compile-time mapping vs. runtime configuration
- Variable naming convention
- Documentation of hash → I/O mapping

### 4. Implement Analog Input Module
- Read A0-A15 using `analogRead()`
- Periodic sampling strategy (e.g., 100Hz per channel)
- Pack raw ADC values (0-1023) as float32
- Send as variable_set frames (0x780 + ecuCanId)
- Handle transmission timing/throttling

### 5. Implement Digital Input Module
- Read D20-D34 using `digitalRead()`
- Pack 15 bits into bitfield variable
- Periodic update or change-detection strategy
- Send packed state via variable_set frame

## Current Questions/Blockers

### Variable Hash Generation
- Do we pre-generate hashes for known variable names?
- Or does epicEFI side define variable names and Mega just uses raw hashes?
- Need hash documentation or generation tool output

### Update Rate Strategy
- Analog inputs: Send every read? Throttle to N Hz? On-change only?
- Digital inputs: Poll rate? Debouncing needed?
- PWM outputs: How often to poll ECU for updates?
- Balance between CAN bus utilization and latency

### Error Handling
- What to do on CAN send failure?
- How to handle missing/invalid frames from ECU?
- Watchdog for ECU communication health?

## Recent Decisions
- **CAN Speed:** 500 kbps selected (per requirements)
- **CS Pin:** D9 reserved for MCP_CAN (standard shield configuration)
- **Serial Debug:** 115200 baud for development/debugging

## Known Issues
- Current code has no CAN transmit capability
- No I/O initialization
- No protocol state machine
- No error handling or recovery

## Testing Needs
- CAN bus hardware validation
- epicEFI ECU integration testing
- I/O electrical verification (analog voltage ranges, digital logic levels)
- Performance testing (frames/sec sustained, latency measurements)

## Documentation Gaps
- Variable hash mapping not yet defined
- Pin assignment verification checklist needed
- Wiring diagrams/schematics missing
- Calibration procedures undefined

