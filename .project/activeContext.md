# Active Context: MEGA_EPIC_CANBUS

## Current Status
**Phase:** Phase 1 Complete - Analog, Digital, VSS I/O Transmission + Smart TX + GPS  
**Last Updated:** Current

## Current Implementation
Firmware (`mega_epic_canbus.ino`) implements:
- MCP2515 CAN controller integration at 500 kbps (Playing With Fusion `PWFusion_MCP2515` library)
- CAN transmission capability (variable_set frames)
- Analog inputs (A0-A15): Periodic sampling, float32 conversion, CAN transmission with smart TX
- Digital inputs (D22-D37): 16-bit bitfield packing, inverted logic, CAN transmission with smart TX
- VSS inputs (D18-D21): Interrupt-driven edge counting, rate calculation, CAN transmission with smart TX
- GPS over Serial2 (GT‑U7 / NEO‑6M class):
  - Reads standard NMEA‑0183 (`GPRMC` + `GPGGA`) at a configurable rate (default 115200 baud, 20 Hz for GT‑U7)
  - Parses time/date, position (lat/lon), speed, course, altitude, fix quality, satellite count
  - Packs time/date into two `uint32_t` variables and sends all GPS values to ECU via CAN using smart TX
- Smart transmission layer: per-channel change detection and adaptive TX intervals (fast on-change, slow heartbeat when stable)
- Big-endian byte conversion utilities
- Overflow-safe counter and time handling

**Code State:**
- Functional CAN TX/RX
- EPIC protocol variable_set frame transmission
- I/O reading and transmission implemented
- VSS interrupt handlers and rate calculation working

## Immediate Next Steps

### 1. Implement EPIC Protocol Frame Parsing
- Parse incoming CAN IDs to identify EPIC frame types
- Implement variable request/response handling
- Implement variable set reception
- Implement function call request/response
- Big-endian byte order conversion utilities

### 2. Design Variable Mapping Scheme
**Critical Decision Needed:**
- How to map Mega I/O to epicEFI variable hashes
- Static compile-time mapping vs. runtime configuration
- Variable naming convention
- Documentation of hash → I/O mapping

### 3. Implement Digital Output Module
- Initialize planned low-speed outputs D39, D40, D41, D42, D43, D47, D48, D49 as OUTPUT
- Request output states from ECU via variable_request
- Unpack bitfield from variable_response
- Apply states via digitalWrite()

### 4. Implement PWM Output Module
- Initialize planned PWM outputs D2, D3, D5, D6, D7, D8, D11, D12, D44, D45, D46
- Define variable hashes and mapping for PWM channels
- Implement duty cycle → timer/PWM configuration

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
- **Serial Debug:** 115200 baud for development/debugging (runtime Serial output currently minimized)
- **ECU CAN ID:** 1 (configured)
- **Digital Inputs:** D22-D37 (16 pins, 16-bit bitfield)
- **VSS Pins:** D18-D21 (INT3, INT2, INT1, INT0) for consistent interrupt behavior
- **VSS Calculation:** 100ms interval for rate calculation
- **Smart TX Strategy:** Inputs sampled at 25 ms; changed values transmitted at 25 ms fast interval; stable values transmitted at ~500 ms heartbeat
- **Pin Planning:** PWM outputs on D2, D3, D5, D6, D7, D8, D11, D12, D44, D45, D46; low-speed outputs on D39, D40, D41, D42, D43, D47, D48, D49; documented in `PINOUT.md`
- **GPS Defaults:** Serial2 at 115200 baud by default, assuming GT‑U7 (u‑blox7) class module capable of 20 Hz NMEA output; code remains compatible with 9600/1–5 Hz receivers by adjusting constants.

## Known Issues
- No EPIC protocol frame parsing (RX only, no handling)
- No digital output implementation on planned low-speed outputs
- No PWM output implementation (timers not yet configured for MEGA_EPIC_* PWM channels)
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

