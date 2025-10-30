# Progress: MEGA_EPIC_CANBUS

**Last Updated:** October 30, 2025

## Implementation Status

### ✅ Completed

#### CAN Bus Infrastructure
- MCP_CAN library integration
- CAN initialization at 500 kbps
- Basic CAN receive loop (polling mode)
- Serial debug output for received frames
- SPI CS pin configuration (D9)

#### Documentation
- Project requirements documented
- EPIC_CAN_BUS protocol specification imported
- Pin allocation defined
- Performance constraints documented

### 🚧 In Progress
- None (awaiting next development phase)

### ❌ Not Started

#### Core Protocol Implementation
- [ ] EPIC protocol frame parser
- [ ] Variable request/response handler
- [ ] Variable set reception
- [ ] Function call request/response
- [ ] Big-endian byte conversion utilities
- [ ] CAN frame transmission (sendMsgBuf)

#### Analog Input Module
- [ ] A0-A15 pin initialization
- [ ] Periodic sampling (analogRead loop)
- [ ] ADC to float32 conversion
- [ ] Variable_set frame composition
- [ ] Transmission scheduling/throttling
- [ ] Variable hash mapping for 16 channels

#### Digital Input Module
- [ ] D20-D34 pin initialization (INPUT_PULLUP)
- [ ] Digital read loop
- [ ] Bitfield packing (15 bits)
- [ ] Variable_set frame transmission
- [ ] Debouncing logic (if needed)

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

### Basic CAN Reception
Current sketch successfully:
- Initializes MCP_CAN at 500 kbps
- Polls for incoming CAN frames
- Reads frame data and CAN ID
- Outputs to Serial monitor (115200 baud)

**Tested:** CAN bus hardware communication verified (receive path)

## What Doesn't Work Yet

### Everything Else
- No CAN transmission capability
- No EPIC protocol parsing or handling
- No I/O reading or control
- No variable mapping
- No error handling

**Current State:** Receive-only skeleton code

## Blockers & Decisions Needed

### Critical Path Items
1. **ecuCanId Selection:** Must choose device CAN ID (0-15) before implementing protocol
2. **Variable Hash Mapping:** Need hash values for analog/digital I/O variables
   - Option A: Pre-generate hashes for known variable names
   - Option B: Receive mapping from ECU at runtime (complex)
   - **Recommendation:** Option A (compile-time mapping)
3. **Update Rate Policy:** Define sampling/polling rates for each I/O type
   - Analog: 10 Hz per channel? 100 Hz?
   - Digital inputs: 20 Hz? Change-detection?
   - PWM parameters: 10 Hz polling?

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
1. **No TX Capability:** Cannot send CAN frames yet
2. **Polling Overhead:** Polling `checkReceive()` wastes CPU cycles
3. **No Error Handling:** Silent failures on invalid frames
4. **No I/O Initialization:** Pins not configured

### Documentation Gaps
1. Variable hash documentation missing (need generated variable list)
2. Wiring diagram for pin connections not created
3. Calibration procedures undefined
4. Testing checklist not written

## Next Milestones

### Milestone 1: Protocol Implementation (Foundation)
- Implement EPIC protocol parser
- Add CAN TX capability
- Create variable hash mapping header
- Test variable_request and variable_response with ECU

### Milestone 2: Analog Inputs (First I/O)
- Initialize A0-A15
- Implement periodic sampling
- Send variable_set frames to ECU
- Verify ECU receives and processes values

### Milestone 3: Digital I/O (Second I/O)
- Initialize D20-D34 (inputs) and D35-D49 (outputs)
- Implement input reading and output writing
- Test bidirectional digital I/O with ECU

### Milestone 4: PWM Outputs (Third I/O)
- Initialize PWM pins
- Poll ECU for PWM parameters
- Apply PWM outputs
- Verify output waveforms

### Milestone 5: Production Readiness
- Error handling and recovery
- Watchdog implementation
- Performance optimization
- Long-term testing
- Documentation completion

## Changelog

### October 30, 2025
- Initial project setup
- Documentation imported
- Basic CAN receive example implemented
- Memory Bank created

