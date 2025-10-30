## MEGA_EPIC_CANBUS

Arduino Mega2560 firmware that expands epicEFI ECU I/O over CAN bus using an MCP2515-based shield. Provides 16 analog inputs, 14 PWM outputs, 15 digital inputs, and 15 digital outputs via the EPIC_CAN_BUS protocol at 500 kbps.

### Features
- **16 analog inputs**: A0–A15 (0–5V)
- **14 PWM outputs**: D2–D8, D10–D13, D44–D46
- **15 digital button inputs**: D20–D34
- **15 digital low-speed outputs**: D35–D49
- **CAN 500 kbps** via MCP2515 (`CS=D9`, SPI on D50–D53)
- EPIC protocol operations: variable request/response, variable set, function call

### Status
- Current: receive-only CAN scaffold prints frames to Serial (115200)
- Missing: EPIC frame parsing, transmit helpers, I/O modules, error handling, `ecuCanId` selection

### Hardware
- Arduino Mega2560
- MCP_CAN shield (MCP2515 + transceiver)
- Wiring:
  - `CS`: D9 (reserved)
  - `SPI`: D50 (MISO), D51 (MOSI), D52 (SCK), D53 (SS)
  - Optional `INT`: D2 for interrupt-driven RX
  - CAN_H/CAN_L twisted pair with proper 120Ω termination

### Pin Map Summary
- **Analog**: A0–A15
- **PWM**: D2, D3, D4, D5, D6, D7, D8, D10, D11, D12, D13, D44, D45, D46 (D9 used by CS)
- **Digital inputs**: D20–D34
- **Digital outputs**: D35–D49

### Protocol (EPIC_CAN_BUS)
- Base IDs (11-bit standard):
  - `0x700 + ecuCanId`: Variable request (DLC=4, int32 hash)
  - `0x720 + ecuCanId`: Variable response (hash + float32 value)
  - `0x740 + ecuCanId`: Function request (uint16 id, float32 arg1, optional int16 arg2)
  - `0x760 + ecuCanId`: Function response (uint16 id, return float32)
  - `0x780 + ecuCanId`: Variable set (hash + float32 value)
- Byte order: big-endian for all multi-byte fields

See `.project/epic_can_bus_spec.txt` for full details.

### Getting Started
1. Install Arduino IDE (1.8.x or 2.x)
2. Libraries:
   - `mcp_canbus` (Seeed/Longan variant)
   - `SPI` (Arduino core)
3. Open `mega_epic_canbus.ino`
4. Board: Arduino Mega or Mega 2560 (ATmega2560)
5. Port: your USB serial port
6. Upload and open Serial Monitor at 115200 baud

### Configuration
- `ecuCanId` (0–15): per-device address used to derive CAN IDs (e.g., `0x700 + ecuCanId`). Define this in code and in docs. If not chosen, default to `1` in early testing.
- `CS` pin: D9 (matches shield default)
- Optional interrupts: attach MCP2515 `INT` to D2 to reduce RX latency

### Repository Structure
- `mega_epic_canbus.ino` — main sketch (setup, basic CAN RX demo)
- `variables.json` — pre-generated variable names and hashes for I/O mapping
- `.project/` — Memory Bank (project intent, architecture, specs)
  - `projectbrief.md` — identity, goals, architecture
  - `productContext.md` — problem/solution overview
  - `activeContext.md` — current work focus and next steps
  - `systemPatterns.md` — architecture, patterns, module plan
  - `techContext.md` — hardware/software constraints
  - `epic_can_bus_spec.txt` — protocol summary

### Roadmap
- Define and store `ecuCanId`
- Implement EPIC frame parsing and TX helpers
- Map variables from `variables.json` to pins (analogs, digital inputs)
- Implement analog input sampling and variable_set TX with throttling
- Implement digital input bitfield TX and output application
- Add PWM output control
- Enable interrupt-driven CAN RX
- Add watchdog and error handling

### Usage (planned behavior)
- Mega periodically samples A0–A15 and sends values as `variable_set` frames
- Digital inputs (D20–D34) packed into a bitfield and sent on change/interval
- ECU responses drive digital outputs (D35–D49) and PWM pins

### Performance Targets
- 500 kbps CAN
- 400–700 frames/sec practical throughput
- <10 ms latency for critical I/O updates

### Contributing
Issues and PRs welcome. Keep changes modular and avoid dynamic allocation on AVR. Reference the Memory Bank docs in `.project/` for architecture and constraints.

### License
TBD. If you intend to contribute a license, add a `LICENSE` file and update this section.


