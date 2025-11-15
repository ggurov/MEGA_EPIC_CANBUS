# Active Context: MEGA_EPIC_CANBUS

## Current Status
**Phase:** Core I/O Transmission - Analog & Digital Inputs Operational  
**Last Updated:** December 2024

## Current Implementation
Firmware (`mega_epic_canbus.ino`) now includes:

### Completed Components
- MCP_CAN library integration with 500 kbps initialization
- **ECU CAN ID:** Set to 1 (defined as `ECU_CAN_ID`)
- **EPIC Protocol CAN IDs:** All base IDs defined (0x700, 0x720, 0x740, 0x760, 0x780 + ecuCanId)
- **Variable Hash Mapping:** 
  - 16 analog input hashes (A0-A15) as compile-time array `VAR_HASH_ANALOG[16]`
  - 1 digital input bitfield hash (D20-D34) as `VAR_HASH_D20_D34`
- **Big-endian Conversion:** Utilities implemented (`writeInt32BigEndian`, `writeFloat32BigEndian`)
- **CAN Transmission:** `sendVariableSetFrame()` function operational
- **Analog Input Module:** Fully functional
  - Reads A0-A15 via `analogRead()`
  - Transmits raw ADC values (0-1023) as float32 every 25ms
  - All 16 channels sent sequentially as variable_set frames
- **Digital Input Module:** Fully functional
  - Reads D20-D34 via `digitalRead()` with INPUT_PULLUP
  - Packs 15 bits into bitfield (LOW=1, HIGH=0 logic)
  - Transmits as float32 every 25ms
- **Pin Initialization:** Analog and digital input pins configured in `setup()`

**Code State:**
- Functional CAN TX (variable_set frames)
- Working analog and digital input transmission
- Periodic transmission at 40 Hz (25ms interval)
- CAN RX polling implemented but frames not parsed yet
- I/O initialization complete for inputs only

## Immediate Next Steps

### 1. Implement CAN Frame Parsing (RX Path)
- Parse incoming CAN IDs to identify EPIC frame types (0x720, 0x760)
- Extract variable hash and value from variable_response frames
- Extract function ID and arguments from function_response frames
- Validate frame structure (DLC=8, correct byte order)

### 2. Implement Digital Output Module (D35-D49)
- Send variable_request frames for digital output bitfield variable
- Parse variable_response frames to extract bitfield value
- Unpack 15 bits and map to D35-D49 pins
- Configure pins as OUTPUT and apply `digitalWrite()` states
- Periodic polling strategy (e.g., 10-20 Hz)

### 3. Implement PWM Output Module
- Identify variable hash for PWM parameter variable(s)
- Send variable_request for PWM duty cycle values
- Parse variable_response to extract float32 percentage (0-100%)
- Convert percentage to 0-255 range for `analogWrite()`
- Configure PWM pins (D2-D8, D10-D13, D44-D46) and apply outputs
- Periodic polling strategy (e.g., 10-20 Hz per channel or grouped)

### 4. Add Error Handling
- Check CAN TX return codes (`CAN.sendMsgBuf()` success/failure)
- Validate received frame DLC and structure
- Handle malformed or unexpected CAN IDs gracefully
- Consider watchdog timer for ECU communication health

## Current Questions/Blockers

### Variable Hash Mapping for Outputs
- Need variable hash for digital output bitfield (D35-D49)
- Need variable hash(es) for PWM duty cycle values
- Are PWM values per-channel or grouped? Single hash or array?

### Update Rate Optimization
- Current: 40 Hz transmission rate (25ms) sends 17 frames per cycle (16 analog + 1 digital)
- Total TX rate: ~680 frames/sec (within 400-700 practical limit, but leaves little room for ECU→Mega)
- **Consider:** Reduce transmission rate or implement change-detection for analog inputs
- **Balance:** Input freshness vs. CAN bus headroom for outputs

### Digital Input Pullup Behavior
- Current implementation uses INPUT_PULLUP on analog inputs (A0-A15)
- **Issue:** Pullups affect ADC readings (floating pins read ~5V)
- **Decision Needed:** Keep pullups for analog (if used with high-impedance sensors) or change to INPUT mode?

### CAN RX Processing Priority
- Current RX path reads frames but doesn't process them
- **Consider:** Switch to interrupt-driven CAN RX (D2 INT pin) to reduce latency
- **Trade-off:** More complex but better real-time response for outputs

## Recent Decisions
- **ECU CAN ID:** Set to 1 (compile-time constant)
- **Transmission Interval:** 25ms (40 Hz) for both analog and digital inputs
- **Analog Input Format:** Raw ADC counts (0-1023) sent as float32 (no scaling)
- **Digital Input Logic:** Inverted (LOW=1, HIGH=0) to match button grounding convention
- **Variable Hashes:** Pre-generated compile-time constants (static mapping approach chosen)

## Known Issues
- **CAN RX Not Parsed:** Frames are received but not processed (protocol parsing missing)
- **No Output Control:** Digital and PWM outputs not implemented (one-way communication only)
- **No Error Handling:** CAN TX failures not checked, invalid frames not handled
- **Analog Pullups:** INPUT_PULLUP mode on analog pins may cause issues with certain sensors
- **Polling Overhead:** Using `CAN.checkReceive()` polling instead of interrupts (latency/CPU trade-off)

## Testing Needs
- Verify CAN TX frames with bus analyzer (confirm variable_set frame structure)
- Test analog input transmission at various voltage levels
- Test digital input bitfield packing/unpacking
- Integration testing with epicEFI ECU (verify ECU receives and processes values)
- Performance testing (confirm 680 frames/sec sustained TX rate)

## Documentation Gaps
- Variable hash mapping for digital/PWM outputs not yet defined
- CAN RX parsing logic not documented
- Output control flow diagrams needed
- Error handling strategy undefined

