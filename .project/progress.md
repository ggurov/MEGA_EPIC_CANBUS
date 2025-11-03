# Progress: MEGA_EPIC_CANBUS

**Last Updated:** December 2024

## Implementation Status

### ✅ Completed

#### CAN Bus Infrastructure
- MCP_CAN library integration
- CAN initialization at 500 kbps
- Basic CAN receive loop (polling mode)
- Serial debug output for received frames
- SPI CS pin configuration (D9)
- **CAN frame transmission** (`CAN.sendMsgBuf()`)

#### EPIC Protocol Implementation
- ECU CAN ID defined (ECU_CAN_ID = 1)
- All EPIC CAN ID base addresses defined (0x700, 0x720, 0x740, 0x760, 0x780)
- **Big-endian byte conversion utilities** (`writeInt32BigEndian`, `writeFloat32BigEndian`)
- **Variable hash mapping** (compile-time constants for 16 analog inputs + 1 digital input)
- **Variable_set frame transmission** (`sendVariableSetFrame()` function)

#### Analog Input Module
- A0-A15 pin initialization (INPUT_PULLUP mode)
- Periodic sampling via `analogRead()` every 25ms
- ADC to float32 conversion (raw counts 0-1023)
- Variable_set frame composition and transmission
- All 16 channels transmitted sequentially

#### Digital Input Module
- D20-D34 pin initialization (INPUT_PULLUP mode)
- Digital read loop every 25ms
- Bitfield packing (15 bits: bit0=D20 ... bit14=D34)
- Inverted logic (LOW=1, HIGH=0 for grounded buttons)
- Variable_set frame transmission

#### Documentation
- Project requirements documented
- EPIC_CAN_BUS protocol specification imported
- Pin allocation defined
- Performance constraints documented

### 🚧 In Progress
- None (awaiting next development phase)

### ❌ Not Started

#### Core Protocol Implementation (RX Path)
- [ ] EPIC protocol frame parser (parse incoming CAN IDs)
- [ ] Variable request handler (send requests to ECU)
- [ ] Variable response parser (extract values from 0x720 frames)
- [ ] Function call request handler
- [ ] Function response parser (extract results from 0x760 frames)
- [ ] Variable set reception (optional - if ECU sends variable_set to Mega)

#### Analog Input Module
- [x] A0-A15 pin initialization ✅
- [x] Periodic sampling (analogRead loop) ✅
- [x] ADC to float32 conversion ✅
- [x] Variable_set frame composition ✅
- [x] Transmission scheduling/throttling (25ms = 40 Hz) ✅
- [x] Variable hash mapping for 16 channels ✅
- [ ] Change-detection optimization (optional, reduce CAN traffic)
- [ ] Calibration/scaling (optional, raw ADC currently sent)

#### Digital Input Module
- [x] D20-D34 pin initialization (INPUT_PULLUP) ✅
- [x] Digital read loop ✅
- [x] Bitfield packing (15 bits) ✅
- [x] Variable_set frame transmission ✅
- [ ] Debouncing logic (if needed - currently direct reads)
- [ ] Change-detection optimization (optional, transmit only on change)

#### Digital Output Module
- [ ] D35-D49 pin initialization (OUTPUT)
- [ ] Variable request for output states
- [ ] Bitfield unpacking
- [ ] digitalWrite application
- [ ] Polling/update strategy

#### PWM Output Module
- [ ] D2-D8, D10-D13, D44-D46 pin initialization
- [ ] Variable request for PWM parameters
- [ ] Percentage to 0-255 conversion
- [ ] analogWrite application
- [ ] Per-channel update scheduling

#### Error Handling & Recovery
- [ ] CAN TX failure handling
- [ ] RX buffer overflow detection
- [ ] Bus-off recovery
- [ ] Watchdog timer for ECU communication
- [ ] Safe mode outputs (fallback state)

#### Testing & Validation
- [ ] Hardware validation (pin voltage levels)
- [ ] CAN bus analyzer verification
- [ ] Integration test with epicEFI ECU
- [ ] Performance benchmarking (frames/sec)
- [ ] Latency measurements
- [ ] Long-term stability testing

#### Future Enhancements
- [ ] Interrupt-driven CAN RX (reduce latency)
- [ ] Interrupt counters on D18-D21
- [ ] Asynchronous ADC reads
- [ ] EEPROM configuration storage
- [ ] Runtime variable hash configuration
- [ ] Modular code structure (separate .h/.cpp files)

## What Works

### CAN Bus Communication
- Initializes MCP_CAN at 500 kbps ✅
- CAN frame transmission operational ✅
- Polls for incoming CAN frames (RX path working) ✅

### Analog Input Transmission
- Reads all 16 analog inputs (A0-A15) ✅
- Converts ADC values (0-1023) to float32 ✅
- Transmits as variable_set frames every 25ms ✅
- All channels sent sequentially (16 frames per cycle) ✅

### Digital Input Transmission
- Reads all 15 digital inputs (D20-D34) ✅
- Packs into 15-bit bitfield ✅
- Transmits as single variable_set frame every 25ms ✅
- Inverted logic correctly implemented (LOW=1) ✅

**Current TX Rate:** ~680 frames/sec (17 frames × 40 Hz = 680 fps)

## What Doesn't Work Yet

### CAN RX Processing
- Frames are received but not parsed
- No EPIC protocol frame type identification
- No variable_response handling
- No function_response handling

### Output Control
- Digital outputs (D35-D49) not initialized or controlled
- PWM outputs (D2-D8, D10-D13, D44-D46) not initialized or controlled
- No variable_request polling for ECU values
- One-way communication only (Mega → ECU, not ECU → Mega)

### Error Handling
- No CAN TX failure detection
- No frame validation
- No error recovery mechanisms
- No watchdog for ECU communication health

**Current State:** Functional TX-only implementation with analog and digital input transmission

## Blockers & Decisions Needed

### Critical Path Items
1. ~~**ecuCanId Selection:**~~ ✅ **RESOLVED** - Set to 1
2. ~~**Variable Hash Mapping:**~~ ✅ **RESOLVED** - Pre-generated compile-time hashes for inputs
   - ✅ Analog input hashes (A0-A15) defined
   - ✅ Digital input hash (D20-D34) defined
   - ❌ **Still Needed:** Digital output hash (D35-D49)
   - ❌ **Still Needed:** PWM output hash(es) (single or per-channel)
3. ~~**Update Rate Policy:**~~ ✅ **RESOLVED** - 25ms interval (40 Hz) for inputs
   - ❌ **Still Needed:** Polling rate for output variable requests (10-20 Hz recommended)
4. **CAN RX Parsing:** Need to implement frame type identification and value extraction

### Technical Decisions
- **Interrupt vs. Polling:** Should implement interrupt-driven CAN RX (D2 INT pin)
- **Modularization:** When to split into multiple files (now or after proof-of-concept?)
- **Error Handling:** How aggressive should watchdog/recovery be?

## Performance Metrics

### Target Metrics
- **Analog Input Latency:** <50 ms sensor-to-CAN
- **Digital Input Latency:** <20 ms button-to-CAN
- **Digital Output Latency:** <30 ms CAN-to-pin
- **PWM Update Latency:** <50 ms CAN-to-output
- **CAN Throughput:** 400+ frames/sec sustained
- **Frame Loss:** 0% under normal operation

### Current Metrics
- No measurements yet (I/O not implemented)
- CAN RX polling working (latency not measured)

## Known Issues

### Current Code Issues
1. ~~**No TX Capability:**~~ ✅ **FIXED** - CAN transmission implemented
2. **Polling Overhead:** Polling `checkReceive()` wastes CPU cycles (consider interrupt-driven RX)
3. **No Error Handling:** CAN TX return codes not checked, invalid frames not handled
4. ~~**No I/O Initialization:**~~ ✅ **FIXED** - Input pins configured (outputs still missing)
5. **Analog Input Pullups:** INPUT_PULLUP mode may cause incorrect readings on floating pins
6. **High TX Rate:** 680 frames/sec leaves little bandwidth for ECU→Mega communication

### Documentation Gaps
1. ~~Variable hash documentation missing~~ ✅ **RESOLVED** - Input hashes documented in code
2. Variable hash mapping for outputs still needed
3. Wiring diagram for pin connections not created
4. Calibration procedures undefined (analog scaling, if needed)
5. Testing checklist not written
6. CAN RX parsing logic not documented

## Next Milestones

### Milestone 1: Protocol Implementation (Foundation)
- ~~Implement EPIC protocol parser~~ ✅ **Partial** - TX path complete
- ~~Add CAN TX capability~~ ✅ **COMPLETE**
- ~~Create variable hash mapping header~~ ✅ **COMPLETE** - Hashes defined in main file
- ❌ Test variable_request and variable_response with ECU (RX parsing needed)

### Milestone 2: Analog Inputs (First I/O)
- ~~Initialize A0-A15~~ ✅ **COMPLETE**
- ~~Implement periodic sampling~~ ✅ **COMPLETE**
- ~~Send variable_set frames to ECU~~ ✅ **COMPLETE**
- ❌ Verify ECU receives and processes values (integration testing pending)

### Milestone 3: Digital I/O (Second I/O)
- ~~Initialize D20-D34 (inputs)~~ ✅ **COMPLETE**
- ~~Implement input reading~~ ✅ **COMPLETE**
- ❌ Initialize D35-D49 (outputs) - **NEXT PRIORITY**
- ❌ Implement output writing from ECU variable_response
- ❌ Test bidirectional digital I/O with ECU

### Milestone 4: PWM Outputs (Third I/O)
- ❌ Initialize PWM pins
- ❌ Poll ECU for PWM parameters via variable_request
- ❌ Parse PWM values from variable_response
- ❌ Apply PWM outputs via `analogWrite()`
- ❌ Verify output waveforms

### Milestone 5: Production Readiness
- ❌ Error handling and recovery
- ❌ Watchdog implementation
- ❌ Performance optimization (reduce TX rate or optimize)
- ❌ Long-term testing
- ❌ Documentation completion

## Changelog

### December 2024
- **MAJOR PROGRESS:** Implemented analog and digital input transmission
- Added ECU_CAN_ID definition (set to 1)
- Implemented big-endian conversion utilities
- Added variable hash arrays for 16 analog inputs
- Added variable hash for digital input bitfield
- Implemented `sendVariableSetFrame()` function
- Implemented 25ms periodic transmission (40 Hz)
- Analog input module: reads and transmits all 16 channels
- Digital input module: reads and transmits 15-bit bitfield
- Pin initialization for A0-A15 and D20-D34
- Total TX rate: ~680 frames/sec

### October 30, 2025
- Initial project setup
- Documentation imported
- Basic CAN receive example implemented
- Memory Bank created

