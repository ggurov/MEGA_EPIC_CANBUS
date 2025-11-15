# Project Brief: MEGA_EPIC_CANBUS

## Project Identity
**Name:** MEGA_EPIC_CANBUS  
**Platform:** Arduino Mega2560 with MCP_CAN Shield  
**Primary Goal:** Expand epicEFI ECU I/O capabilities via CAN bus communication

## Core Purpose
Develop firmware for Arduino Mega2560 paired with MCP_CAN shield to enable seamless communication with epicEFI ECU firmware via CAN bus. This significantly expands input/output capabilities for advanced automotive/embedded applications at budget prices using widely available, accessible hardware.

## Communication Architecture
- **Protocol:** EPIC_CAN_BUS (epicEFI CAN extension protocol)
- **Bus Speed:** 500 kbps
- **ECU ID:** TBD (0-15 range, standard 11-bit CAN IDs)
- **Primary Operations:**
  - Variable Request/Response (read ECU values)
  - Variable Set (write ECU configuration)
  - Function Call (execute ECU functions)

## I/O Capabilities
### Analog Inputs (16 pins)
- Pins A0-A15
- 0-5V native range
- Read via `analogRead()`, send to ECU as variable_set frames

### PWM Outputs (14 pins)
- D2, D3, D4, D5, D6, D7, D8, D10, D11, D12, D13
- D44, D45, D46
- Hardware PWM capable
- D9 reserved for MCP_CAN CS

### Digital Button Inputs (16 pins)
- D22-D37
- Read via `digitalRead()`, pack as 16-bit bitfield, send to ECU

### Digital Low-Speed Outputs (15 pins)
- D35-D49
- Driven via `digitalWrite()` based on ECU variable states

### VSS (Vehicle Speed Sensor) Inputs (4 pins)
- D18, D19, D20, D21
- External interrupt capable (INT3, INT2, INT1, INT0)
- Falling edge detection for VR conditioner compatibility
- Internal pullups enabled to prevent interrupt storms when floating
- Count edges, convert to pulses per second, transmit to ECU

### Reserved/Special Purpose
- D9: MCP_CAN shield chip select (CS)
- D50-D53: SPI bus (MISO, MOSI, SCK, SS) for MCP_CAN
- D14-D17: UART2/UART3, available for GPIO if not used

## Implementation Phases
1. **Phase 1:** Basic CAN communication, analog input transmission
2. **Phase 2:** Digital I/O (buttons and outputs)
3. **Phase 3:** PWM output control
4. **Phase 4:** Interrupt-based counters (wheel speed, etc.)

## Performance Constraints
- SPI clock: 8 MHz max (ATmega2560 limitation)
- CAN throughput: 400-700 frames/sec practical at 500 kbps
- MCP2515 has 2 RX buffers (frame loss risk if not read quickly)
- Single-core, 16 MHz processor with no DMA

## Target Use Case
Budget-friendly automotive ECU I/O expansion using common, understandable hardware that anyone can obtain and work with.

