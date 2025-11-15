# Development Plan: MEGA_EPIC_CANBUS

**Last Updated:** December 2024  
**Status:** Active Development - Phase 1 Complete

## Executive Summary

This document outlines the complete development roadmap for MEGA_EPIC_CANBUS, including:
- Full feature implementation phases
- Architecture design for extensibility
- Branch strategy for multi-platform expansion
- Integration with other epicEFI-compatible systems

## Branch Strategy

### Branch Structure
```
main (current)
├── Core Arduino Mega2560 implementation
├── Full I/O functionality
└── Production-ready firmware

expansion (new branch)
├── Multi-platform support architecture
├── Platform abstraction layer
├── Other epicEFI system implementations
│   ├── ESP32 variant
│   ├── STM32 variant
│   ├── Teensy variant
│   └── Other platforms (future)
└── Shared protocol and communication libraries
```

### Branch Purpose

**`main` branch:**
- Focused on Arduino Mega2560 + MCP_CAN implementation
- Full-featured, production-ready firmware
- Complete I/O module implementation
- Hardware-specific optimizations
- Target: Complete, tested, documented firmware

**`expansion` branch:**
- Multi-platform architecture design
- Platform abstraction layer for code reuse
- Implementation for other epicEFI-compatible systems
- Shared EPIC protocol library
- Target: Extensible framework for various hardware platforms

## Development Phases

### Phase 1: Core I/O Transmission ✅ COMPLETE
**Status:** Implemented  
**Completion Date:** December 2024

**Deliverables:**
- ✅ CAN bus initialization (500 kbps)
- ✅ EPIC protocol CAN ID definitions
- ✅ Big-endian conversion utilities
- ✅ Variable hash mapping (analog + digital inputs)
- ✅ CAN TX capability (`sendVariableSetFrame`)
- ✅ Analog input module (A0-A15, 40 Hz transmission)
- ✅ Digital input module (D20-D34, bitfield packing)

**Metrics Achieved:**
- CAN TX rate: ~680 frames/sec
- Transmission interval: 25ms (40 Hz)
- All 16 analog channels + 15 digital inputs operational

---

### Phase 2: CAN RX Processing & Output Control (Current Priority)
**Status:** In Progress  
**Target Completion:** Next milestone

**Objectives:**
1. Implement EPIC protocol frame parsing (RX path)
2. Add variable request/response handling
3. Implement digital output control (D35-D49)
4. Add basic error handling

**Tasks:**

#### 2.1 CAN Frame Parser
- [ ] Parse incoming CAN IDs to identify frame type
- [ ] Validate frame structure (DLC, byte order)
- [ ] Extract variable hash from variable_response (0x720)
- [ ] Extract function ID/args from function_response (0x760)
- [ ] Handle unknown/malformed frames gracefully

#### 2.2 Variable Request/Response
- [ ] Implement `sendVariableRequest()` function
- [ ] Track pending requests (request-response matching)
- [ ] Parse variable_response frames
- [ ] Extract float32 values from responses
- [ ] Timeout handling for missing responses

#### 2.3 Digital Output Module
- [ ] Define variable hash for digital output bitfield (D35-D49)
- [ ] Initialize D35-D49 pins as OUTPUT
- [ ] Periodic variable_request for output states (10-20 Hz)
- [ ] Parse bitfield from variable_response
- [ ] Unpack 15 bits and apply via `digitalWrite()`
- [ ] Test bidirectional digital I/O

#### 2.4 Basic Error Handling
- [ ] Check CAN TX return codes
- [ ] Validate received frame DLC (must be 8 for EPIC frames)
- [ ] Handle CAN bus errors gracefully
- [ ] Log errors to Serial (debug mode)

**Success Criteria:**
- Digital outputs respond to ECU variable changes within 50ms
- Zero frame parsing errors on valid EPIC frames
- Graceful handling of malformed/invalid frames

---

### Phase 3: PWM Output Module
**Status:** Not Started  
**Target:** After Phase 2

**Objectives:**
1. Implement PWM output control from ECU
2. Poll ECU for PWM duty cycle parameters
3. Apply PWM outputs with proper timing

**Tasks:**

#### 3.1 PWM Variable Mapping
- [ ] Define variable hash strategy for PWM values
  - Option A: Single hash with array of 14 values
  - Option B: Individual hash per channel (14 hashes)
  - **Decision Needed:** Check epicEFI variable structure
- [ ] Document PWM pin assignments (D2-D8, D10-D13, D44-D46)

#### 3.2 PWM Request/Response Loop
- [ ] Initialize PWM pins (configure for PWM mode)
- [ ] Periodic variable_request for PWM parameters (10-20 Hz)
- [ ] Parse PWM values from variable_response (float32, 0-100%)
- [ ] Convert percentage to 0-255 range for `analogWrite()`
- [ ] Apply PWM outputs

#### 3.3 PWM Timing Optimization
- [ ] Consider grouped requests (single frame with multiple hashes if supported)
- [ ] Batch updates to reduce CAN traffic
- [ ] Handle PWM update timing (avoid flicker)

**Success Criteria:**
- All 14 PWM channels operational
- PWM updates within 100ms of ECU change
- Smooth PWM transitions without flicker

---

### Phase 4: Function Call Support
**Status:** Not Started  
**Priority:** Medium (if needed by ECU)

**Objectives:**
1. Implement function call request/response
2. Support ECU function execution from Mega
3. Handle function arguments and return values

**Tasks:**

#### 4.1 Function Call Frame Composition
- [ ] Implement `sendFunctionRequest()` function
- [ ] Pack function ID (uint16), argument 1 (int32), argument 2 (int16)
- [ ] Send on CAN ID 0x740 + ecuCanId

#### 4.2 Function Response Handling
- [ ] Parse function_response frames (0x760 + ecuCanId)
- [ ] Extract function ID and return value
- [ ] Match responses to requests (request-response pairing)
- [ ] Handle function call timeouts

#### 4.3 Use Cases
- [ ] Trigger ECU calibration routines
- [ ] Request ECU status/health checks
- [ ] Execute ECU diagnostic functions

**Success Criteria:**
- Function calls execute successfully
- Return values correctly parsed
- Timeout handling for missing responses

---

### Phase 5: Performance Optimization
**Status:** Not Started  
**Priority:** High (address CAN bus bandwidth)

**Objectives:**
1. Optimize CAN bus utilization
2. Reduce transmission overhead
3. Improve latency for critical I/O

**Tasks:**

#### 5.1 Change Detection for Analog Inputs
- [ ] Track previous ADC values for each channel
- [ ] Transmit only on significant change (configurable threshold)
- [ ] Periodic "heartbeat" transmission even if unchanged
- [ ] Reduce CAN traffic by 50-80% for static sensors

#### 5.2 Change Detection for Digital Inputs
- [ ] Track previous bitfield state
- [ ] Transmit only on state change
- [ ] Immediate transmission for critical buttons

#### 5.3 Interrupt-Driven CAN RX
- [ ] Configure MCP_CAN INT pin (D2) for interrupts
- [ ] Replace polling with interrupt service routine
- [ ] Reduce CAN RX latency (critical for outputs)
- [ ] Improve CPU efficiency

#### 5.4 Transmission Rate Optimization
- [ ] Analyze current 680 frames/sec usage
- [ ] Reduce transmission rate if bandwidth constrained
- [ ] Prioritize critical inputs (higher rate)
- [ ] De-prioritize static inputs (lower rate)

**Success Criteria:**
- CAN TX rate reduced to <400 frames/sec average
- Critical I/O latency <20ms
- CPU usage optimized (more headroom for future features)

---

### Phase 6: Error Handling & Robustness
**Status:** Not Started  
**Priority:** High (production readiness)

**Objectives:**
1. Comprehensive error handling
2. Communication health monitoring
3. Safe mode operation
4. Recovery mechanisms

**Tasks:**

#### 6.1 CAN Bus Error Handling
- [ ] Detect CAN TX failures (`sendMsgBuf` return codes)
- [ ] Retry logic for transient failures
- [ ] Bus-off detection and recovery
- [ ] MCP2515 error flag monitoring

#### 6.2 ECU Communication Watchdog
- [ ] Track last successful variable_response timestamp
- [ ] Detect ECU communication loss (timeout: 2-5 seconds)
- [ ] Enter safe mode on communication loss
  - Digital outputs: safe state (all LOW or configurable)
  - PWM outputs: safe duty cycle (0% or configurable)
  - Maintain input transmission (keep trying)

#### 6.3 Frame Validation
- [ ] Validate all received frames (DLC, CAN ID range)
- [ ] Validate EPIC protocol structure (hash format, value ranges)
- [ ] Reject malformed frames with logging

#### 6.4 Diagnostic Features
- [ ] CAN bus statistics (TX/RX counts, errors)
- [ ] Frame loss detection (if possible)
- [ ] Communication health indicators
- [ ] Serial debug output for troubleshooting

**Success Criteria:**
- Graceful degradation on communication loss
- Zero crashes on invalid frames
- Automatic recovery from transient errors
- Clear diagnostic information

---

### Phase 7: Advanced Features
**Status:** Not Started  
**Priority:** Low (nice-to-have)

**Objectives:**
1. Interrupt-driven counters (wheel speed, etc.)
2. ADC calibration/scaling
3. Configuration storage
4. Modular code structure

**Tasks:**

#### 7.1 Interrupt Counters
- [ ] Configure D18-D21 for external interrupts
- [ ] Implement counter ISRs
- [ ] Calculate frequency/delta counts
- [ ] Transmit counter values via variable_set
- [ ] Variable hash mapping for counters

#### 7.2 ADC Calibration
- [ ] Optional scaling from raw ADC (0-1023) to engineering units
- [ ] Per-channel offset and gain calibration
- [ ] Store calibration in EEPROM
- [ ] Send calibrated values instead of raw ADC

#### 7.3 EEPROM Configuration
- [ ] Store ECU_CAN_ID in EEPROM (configurable at runtime)
- [ ] Store variable hash mappings (if made configurable)
- [ ] Store calibration parameters
- [ ] Configuration utility functions

#### 7.4 Code Modularization
- [ ] Split into separate .h/.cpp files:
  - `epic_protocol.h/.cpp` - EPIC protocol handling
  - `analog_inputs.h/.cpp` - Analog input management
  - `digital_io.h/.cpp` - Digital input/output management
  - `pwm_outputs.h/.cpp` - PWM output management
  - `variable_map.h` - Variable hash mappings
  - `can_bus.h/.cpp` - CAN bus wrapper (if needed)
- [ ] Maintain single-file option for simplicity (optional)

**Success Criteria:**
- Modular code structure (easier maintenance)
- Optional advanced features (interrupt counters, calibration)
- Flexible configuration

---

### Phase 8: Testing & Validation
**Status:** Not Started  
**Priority:** High (all phases)

**Objectives:**
1. Comprehensive hardware testing
2. Integration testing with epicEFI ECU
3. Performance benchmarking
4. Long-term stability testing

**Tasks:**

#### 8.1 Hardware Validation
- [ ] Verify all pin assignments (analog, digital, PWM)
- [ ] Test analog input voltage ranges (0-5V)
- [ ] Test digital input logic levels (LOW/HIGH thresholds)
- [ ] Verify PWM output waveforms (scope testing)
- [ ] Test CAN bus physical layer (termination, signal quality)

#### 8.2 Integration Testing
- [ ] Test with live epicEFI ECU on CAN bus
- [ ] Verify variable_set frames received by ECU
- [ ] Verify variable_response frames from ECU processed correctly
- [ ] Test bidirectional I/O (inputs to ECU, outputs from ECU)
- [ ] End-to-end functional testing

#### 8.3 Performance Testing
- [ ] Measure CAN frame rates (TX and RX)
- [ ] Measure I/O latency (sensor → CAN, CAN → output)
- [ ] Test sustained operation (hours/days)
- [ ] Test under load (maximum frame rates)
- [ ] Frame loss rate measurement

#### 8.4 Edge Case Testing
- [ ] CAN bus disconnection/reconnection
- [ ] Invalid frame handling
- [ ] ECU reboot scenarios
- [ ] Extreme temperature testing (if applicable)
- [ ] Power supply variations

**Success Criteria:**
- All hardware functions verified
- Successful integration with epicEFI ECU
- Performance metrics within targets
- Zero frame loss under normal operation
- Stable operation for 24+ hours continuous

---

### Phase 9: Documentation & Production Readiness
**Status:** Not Started  
**Priority:** High (release preparation)

**Objectives:**
1. Complete user documentation
2. Setup/configuration guide
3. Troubleshooting guide
4. Release preparation

**Tasks:**

#### 9.1 User Documentation
- [ ] README.md with overview and quick start
- [ ] Pin assignment diagram/table
- [ ] Wiring diagrams for common use cases
- [ ] Variable hash reference document
- [ ] Configuration guide

#### 9.2 Technical Documentation
- [ ] Architecture overview
- [ ] Protocol implementation details
- [ ] Code structure and organization
- [ ] Development guide (for contributors)
- [ ] API documentation (if modularized)

#### 9.3 Troubleshooting Guide
- [ ] Common issues and solutions
- [ ] CAN bus debugging procedures
- [ ] Serial debug output interpretation
- [ ] Hardware verification checklist
- [ ] Contact information / support channels

#### 9.4 Release Preparation
- [ ] Version numbering scheme
- [ ] Release notes template
- [ ] Binary firmware releases (optional)
- [ ] Installation instructions
- [ ] License clarification (if needed)

**Success Criteria:**
- Complete documentation for end users
- Clear setup instructions
- Troubleshooting resources available
- Ready for public release

---

## Architecture for Extensibility

### Platform Abstraction Layer (for `expansion` branch)

**Design Goal:** Allow code reuse across different hardware platforms while maintaining platform-specific optimizations.

#### Proposed Structure

```
expansion/
├── core/
│   ├── epic_protocol.h/.cpp      # EPIC protocol handling (platform-agnostic)
│   ├── variable_map.h             # Variable hash mappings (shared)
│   └── protocol_types.h           # Common types and constants
│
├── platforms/
│   ├── arduino_mega/              # Current Mega2560 implementation
│   │   ├── platform_config.h
│   │   ├── can_bus.h/.cpp         # MCP_CAN wrapper
│   │   ├── analog_io.h/.cpp       # Platform-specific analog I/O
│   │   ├── digital_io.h/.cpp      # Platform-specific digital I/O
│   │   └── pwm_io.h/.cpp          # Platform-specific PWM I/O
│   │
│   ├── esp32/                     # Future ESP32 variant
│   │   ├── platform_config.h
│   │   ├── can_bus.h/.cpp         # ESP32 CAN controller
│   │   └── [I/O modules]
│   │
│   └── stm32/                     # Future STM32 variant
│       ├── platform_config.h
│       ├── can_bus.h/.cpp         # STM32 CAN peripheral
│       └── [I/O modules]
│
└── examples/
    ├── mega_basic/
    ├── esp32_basic/
    └── [platform examples]
```

#### Abstraction Interfaces

```cpp
// Platform-agnostic CAN interface
class CANBus {
public:
    virtual bool begin(uint32_t baudrate) = 0;
    virtual bool sendFrame(uint32_t canId, uint8_t* data, uint8_t len) = 0;
    virtual bool receiveFrame(uint32_t* canId, uint8_t* data, uint8_t* len) = 0;
    virtual bool checkReceive() = 0;
};

// Platform-specific implementations
class MCP_CANBus : public CANBus { /* MCP2515 */ };
class ESP32_CANBus : public CANBus { /* ESP32 CAN */ };
class STM32_CANBus : public CANBus { /* STM32 CAN */ };

// Platform-agnostic I/O interfaces
class AnalogInput {
public:
    virtual uint16_t read(uint8_t channel) = 0;
};

class DigitalIO {
public:
    virtual void setMode(uint8_t pin, PinMode mode) = 0;
    virtual bool read(uint8_t pin) = 0;
    virtual void write(uint8_t pin, bool state) = 0;
};
```

### Shared EPIC Protocol Library

**Location:** `expansion/core/epic_protocol.h/.cpp`

**Purpose:** Reusable EPIC protocol implementation independent of hardware platform.

**Contents:**
- Big-endian conversion utilities
- Frame composition functions (variable_request, variable_set, function_request)
- Frame parsing functions (variable_response, function_response)
- Protocol constants and type definitions

**Benefits:**
- Single source of truth for protocol implementation
- Easier to maintain and update
- Consistent behavior across platforms
- Reduced code duplication

### Platform-Specific Optimizations

Each platform implementation can:
- Leverage hardware-specific features (DMA, interrupts, dedicated peripherals)
- Optimize for platform constraints (memory, CPU speed)
- Use platform-native libraries and APIs
- Maintain platform-specific pin assignments

## Expansion Targets (Future Platforms)

### ESP32 Variant
**Advantages:**
- Built-in CAN controller (no external chip needed)
- More processing power and memory
- Wi-Fi/Bluetooth capabilities (potential remote monitoring)
- Lower cost than Mega + shield combination

**Considerations:**
- Different pin assignments
- Different ADC resolution/characteristics
- PWM implementation differences
- Additional capabilities (Wi-Fi, etc.)

### STM32 Variant
**Advantages:**
- Industrial-grade reliability
- Multiple CAN controllers available
- Rich peripheral set
- Professional automotive use cases

**Considerations:**
- More complex development environment
- Different toolchain (STM32CubeIDE)
- Professional/industrial focus

### Teensy Variant
**Advantages:**
- High performance (240 MHz+ options)
- Excellent Arduino compatibility
- Built-in CAN on some models
- Active community

**Considerations:**
- Cost (typically $20-60)
- Pin count varies by model

## Development Timeline

### Current Phase (Phase 2)
**Estimated Duration:** 2-4 weeks
- CAN RX parsing: 1 week
- Digital output module: 1 week
- Testing and refinement: 1-2 weeks

### Phase 3 (PWM Outputs)
**Estimated Duration:** 1-2 weeks

### Phase 4 (Function Calls)
**Estimated Duration:** 1 week (if needed)

### Phase 5 (Performance Optimization)
**Estimated Duration:** 2-3 weeks

### Phase 6 (Error Handling)
**Estimated Duration:** 2 weeks

### Phase 7 (Advanced Features)
**Estimated Duration:** 2-4 weeks (optional, lower priority)

### Phase 8 (Testing)
**Estimated Duration:** Ongoing, 2-4 weeks intensive testing

### Phase 9 (Documentation)
**Estimated Duration:** 1-2 weeks

**Total Estimated Duration (Phases 2-9):** 12-22 weeks (3-5.5 months)

**Note:** Timeline is approximate and depends on available development time, testing resources, and ECU integration availability.

## Success Metrics

### Functional Requirements
- [ ] All 16 analog inputs operational
- [ ] All 15 digital inputs operational
- [ ] All 15 digital outputs operational
- [ ] All 14 PWM outputs operational
- [ ] Bidirectional communication with ECU
- [ ] Error handling and recovery
- [ ] Stable long-term operation

### Performance Requirements
- [ ] CAN bus utilization <70% (leave headroom)
- [ ] I/O latency <50ms (sensor → CAN, CAN → output)
- [ ] Frame loss rate <0.1% under normal operation
- [ ] Zero crashes on invalid frames
- [ ] 24+ hour continuous operation stability

### Code Quality Requirements
- [ ] Well-commented code
- [ ] Consistent code style
- [ ] Modular architecture (Phase 7)
- [ ] Comprehensive documentation

## Risk Management

### Technical Risks

**High Bandwidth Usage (680 frames/sec)**
- **Mitigation:** Implement change detection (Phase 5)
- **Contingency:** Reduce transmission rate, prioritize channels

**CAN Bus Errors/Disconnections**
- **Mitigation:** Robust error handling (Phase 6)
- **Contingency:** Watchdog and safe mode operation

**ECU Compatibility Issues**
- **Mitigation:** Early integration testing (Phase 8)
- **Contingency:** Protocol debugging, ECU firmware coordination

### Schedule Risks

**Development Time Overruns**
- **Mitigation:** Prioritize critical features (Phases 2-3, 6)
- **Contingency:** Defer advanced features (Phase 7) if needed

**Testing Resource Availability**
- **Mitigation:** Plan testing phases early
- **Contingency:** Community testing, staged releases

## Next Actions

1. **Immediate (This Week):**
   - Commit Memory Bank updates to `main` branch
   - Create `expansion` branch
   - Begin Phase 2: CAN RX parsing implementation

2. **Short-term (Next 2-4 Weeks):**
   - Complete Phase 2 (RX parsing, digital outputs)
   - Begin Phase 3 (PWM outputs)
   - Initial integration testing with ECU

3. **Medium-term (Next 2-3 Months):**
   - Complete Phases 3-6 (core functionality)
   - Performance optimization
   - Comprehensive testing

4. **Long-term (3-6 Months):**
   - Advanced features (Phase 7)
   - Documentation (Phase 9)
   - `expansion` branch development (multi-platform)

## Notes

- This plan is a living document and should be updated as development progresses
- Phases may overlap (e.g., testing can occur alongside development)
- Priorities may shift based on user feedback and requirements
- `expansion` branch work can proceed in parallel with `main` branch development

