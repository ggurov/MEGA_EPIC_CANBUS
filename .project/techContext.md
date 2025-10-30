# Technical Context: MEGA_EPIC_CANBUS

## Hardware Platform

### Arduino Mega2560
- **MCU:** ATmega2560
- **Clock:** 16 MHz
- **Flash:** 256 KB (8 KB used by bootloader)
- **SRAM:** 8 KB
- **EEPROM:** 4 KB
- **GPIO:** 54 digital pins, 16 analog inputs
- **PWM:** 15 pins (8-bit, ~490 Hz on most, ~980 Hz on D4 and D13)
- **Serial Ports:** 4 hardware UARTs
- **SPI:** 1 hardware SPI bus (pins 50-53)
- **I2C:** 1 hardware I2C bus (pins 20, 21)

### MCP_CAN Shield
- **CAN Controller:** MCP2515 (SPI interface)
- **Transceiver:** TJA1050, MCP2551, or similar (depends on shield variant)
- **CS Pin:** D9 (standard configuration)
- **INT Pin:** Typically D2 (interrupt capable)
- **SPI Pins:** D50 (MISO), D51 (MOSI), D52 (SCK), D53 (SS - unused for MCP_CAN)

### Pin Allocation
**Analog Inputs (16 pins):**
- A0-A15: Analog sensors (0-5V range)

**PWM Outputs (14 pins):**
- D2, D3, D4, D5, D6, D7, D8: PWM capable
- D10, D11, D12, D13: PWM capable
- D44, D45, D46: PWM capable
- D9: Reserved for MCP_CAN CS

**Digital Button Inputs (15 pins):**
- D20-D34: Digital inputs (internal pullup available)

**Digital Low-Speed Outputs (15 pins):**
- D35-D49: Digital outputs

**Reserved:**
- D9: MCP_CAN CS (SPI)
- D50-D53: SPI bus (MCP_CAN)
- D2: MCP_CAN INT (optional, for interrupt-driven RX)

**Interrupt Capable:**
- D2, D3: INT0, INT1
- D18, D19: INT3, INT2 (also UART1 TX/RX)
- D20, D21: INT1, INT0 (also I2C SDA/SCL)

**Available for Future Use:**
- D14-D19: UART2 and UART3 pins (if UARTs not needed)

## Software Stack

### Development Environment
- **IDE:** Arduino IDE 1.8.x or 2.x
- **Toolchain:** avr-gcc (AVR 8-bit C compiler)
- **Upload Protocol:** STK500v2 or AVR109 (via USB-to-serial)
- **Serial Monitor:** 115200 baud (debug output)

### Core Libraries

#### MCP_CAN Library
- **Purpose:** Interface with MCP2515 CAN controller
- **Source:** Seeed Studio or Longan Labs forks (common variants)
- **Key Functions:**
  - `CAN.begin(CAN_500KBPS)` - Initialize CAN at 500 kbps
  - `CAN.checkReceive()` - Poll for received frames
  - `CAN.readMsgBuf(&len, buf)` - Read frame data
  - `CAN.getCanId()` - Get received frame CAN ID
  - `CAN.sendMsgBuf(canId, 0, dlc, data)` - Send CAN frame (standard ID, data frame)
  
**Code Smell:** Library uses polling by default; interrupt-driven mode available but requires additional setup.

#### SPI Library (Arduino Core)
- **Purpose:** Hardware SPI communication (used by MCP_CAN)
- **Speed:** Default ~8 MHz (maximum for ATmega2560)
- **Mode:** Automatically managed by MCP_CAN library

### Language and Syntax
- **Language:** C++ (Arduino dialect)
- **Standard:** C++11 subset (limited by avr-gcc version)
- **Constraints:**
  - No exceptions
  - No STL (no `std::vector`, etc.)
  - Limited dynamic memory allocation (avoid due to fragmentation)
  - No threading or RTOS

## Protocol Details

### EPIC_CAN_BUS Protocol
- **Specification:** `.project/epic_can_bus_spec.txt`
- **CAN ID Type:** 11-bit standard IDs
- **Byte Order:** Big-endian (network order)
- **Data Types:**
  - int32: Variable hashes (signed, two's complement)
  - float32: Variable values (IEEE 754)
  - uint16: Function IDs
  - int16: Function argument 2

**Base CAN IDs:**
- 0x700 + ecuCanId: Variable request
- 0x720 + ecuCanId: Variable response
- 0x740 + ecuCanId: Function request
- 0x760 + ecuCanId: Function response
- 0x780 + ecuCanId: Variable set

**ecuCanId Range:** 0-15 (must be configured per device)

### CAN Bus Physical
- **Speed:** 500 kbps
- **Termination:** 120Ω resistors at each end of bus (verify shield has termination)
- **Cable:** Twisted pair, CAN_H and CAN_L
- **Max Length:** ~100m at 500 kbps (depends on cable quality)

## Performance Constraints

### CAN Bus Throughput
- **Theoretical Max:** ~800 frames/sec at 500 kbps (11-bit ID, 8-byte payload)
- **Practical Mega2560:** 400-700 frames/sec sustained
- **Limiting Factors:**
  - SPI clock limited to 8 MHz
  - MCP2515 internal buffers (2 RX, 3 TX)
  - Polling overhead if not using interrupts
  - Arduino sketch processing time

### Timing Constraints
- **SPI Transaction:** 10-50 µs per read/write
- **CAN Frame RX:** 200-500 µs (SPI overhead + processing)
- **Analog Read:** ~100 µs per channel
- **Digital Read/Write:** <10 µs
- **PWM Update:** <10 µs (`analogWrite()`)

**Implication:** Must process CAN frames quickly to prevent MCP2515 buffer overflow.

### Memory Constraints
- **SRAM:** 8 KB total
  - ~1 KB for stack
  - ~1 KB for global variables
  - ~6 KB available for buffers and runtime data
- **Flash:** 256 KB (plenty for firmware)
- **EEPROM:** 4 KB (unused currently)

**Recommendation:** Avoid dynamic allocation, use static buffers, profile stack usage.

## Development Workflow

### Build and Upload
1. Open `mega_epic_canbus.ino` in Arduino IDE
2. Select Board: "Arduino Mega or Mega 2560"
3. Select Processor: "ATmega2560 (Mega 2560)"
4. Select Port: (USB serial port)
5. Click Upload
6. Monitor Serial output at 115200 baud

### Debugging
- **Serial Debug:** `Serial.println()` statements
- **CAN Monitoring:** Use CAN bus analyzer (e.g., CANable, PEAK PCAN-USB)
- **Oscilloscope:** Verify CAN_H/CAN_L signals if hardware issues suspected
- **Multimeter:** Check power supply (5V) and pin voltages

**Code Smell:** Heavy reliance on Serial debug; consider conditional compilation for production vs. debug builds.

### Testing Strategy
1. **Unit Testing:** Limited (Arduino has no native unit test framework)
2. **Hardware-in-Loop:** Test with live ECU on CAN bus
3. **CAN Bus Analyzer:** Verify frame structure and timing
4. **Bench Testing:** Analog voltage sources, digital button simulation
5. **Integration Testing:** Full system test with ECU in vehicle

## Dependencies

### Required Libraries
- **mcp_canbus:** Seeed Studio or Longan Labs MCP_CAN library
- **SPI:** Arduino core library (built-in)

### Installation
```bash
# Via Arduino Library Manager:
# Sketch → Include Library → Manage Libraries → Search "mcp_can" or "mcp_canbus"

# Or manual installation:
# Download library, extract to ~/Arduino/libraries/
```

## Known Limitations

### Hardware Limitations
- 8 MHz SPI clock (can't go faster on ATmega2560)
- 8 KB SRAM (constrains buffer sizes and complexity)
- Single-core, no DMA (must handle all processing sequentially)
- MCP2515 has only 2 RX buffers (frame loss if not read quickly)

### Software Limitations
- Polling-based CAN RX in current implementation (latency, CPU usage)
- No built-in watchdog timer usage (yet)
- No error recovery for bus-off or persistent errors
- No configuration storage (hard-coded pin mappings)

### Protocol Limitations
- Fire-and-forget variable_set (no ACK confirmation)
- Zero-as-error pattern (ambiguous with legitimate zero values)
- No built-in authentication or security
- No flow control or backpressure mechanism

## Compiler Flags and Optimization
Default Arduino IDE flags:
- `-Os` (optimize for size)
- `-ffunction-sections -fdata-sections` (dead code elimination)
- `-flto` (link-time optimization, in newer IDE versions)

**Consideration:** Profiling with `-O2` or `-O3` may improve performance but increase binary size.

## Future Technical Enhancements
- Interrupt-driven CAN RX (reduce latency and CPU usage)
- Asynchronous ADC reads (free up CPU during conversion)
- DMA-like buffering strategies (within SRAM constraints)
- EEPROM configuration storage (ecuCanId, variable mappings)
- Watchdog timer for fault recovery

