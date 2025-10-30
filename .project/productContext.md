# Product Context: MEGA_EPIC_CANBUS

## Problem Statement

### Limited ECU I/O
Modern engine control units (ECUs) like epicEFI have limited physical I/O due to:
- Package size constraints
- Cost of additional pins and supporting circuitry
- Physical board space limitations
- Fixed I/O count at design time

### Real-World I/O Demands
Advanced automotive applications require extensive I/O:
- Multiple analog sensors (pressure, temperature, position)
- Digital button inputs (dashboard switches, driver controls)
- PWM outputs (boost control solenoids, cooling fans, auxiliary systems)
- Digital outputs (relays, indicators, auxiliary systems)
- Interrupt counters (wheel speed sensors, shaft encoders)

### Existing Solutions Are Expensive
Commercial I/O expansion modules:
- Proprietary protocols
- High cost ($500-2000+)
- Limited availability
- Vendor lock-in
- Difficult to service/repair in field

## Solution: MEGA_EPIC_CANBUS

### Core Concept
Use Arduino Mega2560 + MCP_CAN shield as I/O expansion module communicating with epicEFI ECU via standardized CAN bus protocol.

### Why This Approach Works

**Accessibility:**
- Arduino Mega2560: $10-40, globally available
- MCP_CAN shield: $5-20, common part
- Anyone can source, understand, modify, and repair

**Robust I/O:**
- 16 analog inputs (A0-A15, 0-5V native)
- 14 hardware PWM outputs
- 15 digital button inputs
- 15 digital low-speed outputs
- 6 interrupt-capable pins for counters

**Industry Standard Communication:**
- CAN bus: proven automotive protocol
- 500 kbps: appropriate for I/O expansion use case
- EPIC_CAN_BUS: open protocol from epicEFI project
- Reliable, noise-resistant differential signaling

**Developer Friendly:**
- Arduino IDE: simple, widely known
- Open source libraries (MCP_CAN)
- Easy debugging via serial monitor
- Modifiable by end users

### How It Solves the Problem

1. **Analog Sensors:** Mega reads A0-A15, transmits raw ADC values to ECU via CAN variable_set frames
2. **Digital Buttons:** Mega reads D20-D34, packs states into bitfield variable, ECU polls/receives
3. **PWM Outputs:** ECU stores PWM parameters in variables, Mega requests and applies to D2-D13, D44-D46
4. **Digital Outputs:** ECU stores desired output states, Mega polls and applies via `digitalWrite()` on D35-D49
5. **Counters (Future):** Mega handles interrupt-driven counters on D18-D21, reports deltas/timestamps to ECU

## Target Users

### DIY Engine Builders
Enthusiasts building custom engine management systems who need:
- Flexible I/O expansion
- Budget-conscious solutions
- Understandable, serviceable hardware

### Professional Tuners
Shops needing:
- Cost-effective I/O expansion for customer builds
- Easy field serviceability
- Ability to customize for specific applications

### Educational/Research
Universities and research programs requiring:
- Accessible embedded systems teaching platform
- Real automotive protocols
- Open, modifiable design

## Success Criteria
- Reliable CAN communication at 500 kbps
- <10ms latency for critical I/O updates
- 400+ CAN frames/sec sustained throughput
- Zero data loss under normal operation
- Simple setup and configuration
- Detailed documentation for users
- Open source for community contribution

