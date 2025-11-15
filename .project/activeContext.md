# Active Context: MEGA_EPIC_CANBUS

## Current Status
**Phase:** Phase 1 Complete - Analog, Digital, and VSS I/O Transmission  
**Last Updated:** Current

## Current Implementation
Firmware (`mega_epic_canbus.ino`) implements:
- MCP_CAN library integration at 500 kbps
- CAN transmission capability (variable_set frames)
- Analog inputs (A0-A15): Periodic sampling, float32 conversion, CAN transmission
- Digital inputs (D22-D37): 16-bit bitfield packing, inverted logic, CAN transmission
- VSS inputs (D18-D21): Interrupt-driven edge counting, rate calculation, CAN transmission
- Big-endian byte conversion utilities
- Overflow-safe counter and time handling

**Code State:**
- Functional CAN TX/RX
- EPIC protocol variable_set frame transmission
- I/O reading and transmission implemented
- VSS interrupt handlers and rate calculation working

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

### 5. Implement Digital Output Module
- Initialize D35-D49 as OUTPUT
- Request output states from ECU via variable_request
- Unpack bitfield from variable_response
- Apply states via digitalWrite()

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
- **ECU CAN ID:** 1 (configured)
- **Digital Inputs:** D22-D37 (16 pins, 16-bit bitfield)
- **VSS Pins:** D18-D21 (INT3, INT2, INT1, INT0) for consistent interrupt behavior
- **VSS Calculation:** 100ms interval for rate calculation
- **Transmission Interval:** 25ms for all I/O updates

## Known Issues
- No EPIC protocol frame parsing (RX only, no handling)
- No digital output implementation (D35-D49)
- No PWM output implementation
- No error handling or recovery
- No watchdog timer

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

