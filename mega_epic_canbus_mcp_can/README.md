# MEGA_EPIC_CANBUS - MCP_CAN Library Port

Complete port for the **MCP_CAN library**, compatible with Seeed Studio and SparkFun CAN shields.

## Why This Port?

The original uses PWFusion_MCP2515 library. This version uses the widely-available **MCP_CAN library**:
- ✅ Available in Arduino Library Manager
- ✅ Works with Seeed Studio CAN-BUS Shield V2.0
- ✅ Works with SparkFun CAN-BUS Shield
- ✅ Works with most commercial MCP2515 shields

## Features (100% Parity)

All original features maintained:
- ✅ 16 Analog inputs (A0-A15)
- ✅ 16 Digital inputs (D22-D37)
- ✅ 8 Slow GPIO outputs (D39-D43, D47-D49)
- ✅ 10 PWM outputs
- ✅ 4 VSS (wheel speed) inputs
- ✅ GPS integration (NMEA 0183)
- ✅ EPIC CAN protocol

## Quick Start

1. **Install MCP_CAN library:**
   - Arduino IDE → Sketch → Include Library → Manage Libraries
   - Search "MCP_CAN"
   - Install "MCP_CAN library" by Cory Fowler

2. **Open sketch:**
   - Open `mega_epic_canbus_mcp_can.ino`

3. **Configure hardware:**
   - Seeed Studio shield: PWR_OBD = OFF, CS = D9
   - Set 120Ω termination if at end of bus

4. **Upload:**
   - Board: Arduino Mega 2560
   - Upload!

## Hardware Tested

- Arduino Mega 2560 R3
- Seeed Studio CAN-BUS Shield V2.0
- 16MHz crystal, 500kbps CAN
- Various sensors (fuel pressure, GPS, VSS)

## Key Changes from Original

- **Library:** PWFusion_MCP2515 → MCP_CAN
- **NMEA Parser:** Custom lightweight implementation (no external deps)
- **CAN Handling:** Adapted for MCP_CAN API
- **Everything else:** Identical protocol, same features

## Files

- `mega_epic_canbus_mcp_can.ino` - Main sketch
- `nmea_parser.h` - GPS parser header
- `nmea_parser.cpp` - GPS parser implementation

All 3 files must be in same folder!

## Tested Use Cases

- Fuel pressure monitoring (0-150 PSI sensor on A0)
- GPS data logging
- Wheel speed sensors (4 channels)
- Digital button inputs
- PWM output control

## Troubleshooting

**CAN Init Failed:**
- Check shield is seated properly
- PWR_OBD jumper must be OFF
- Verify 16MHz crystal setting

**No Messages on Bus:**
- Match CAN speed (500kbps default)
- Check 120Ω terminators on both ends
- Verify CAN_H/CAN_L wiring

## Credits

Original MEGA_EPIC_CANBUS by Gennady Gurov  
MCP_CAN port for broader hardware compatibility

## License

Same as original repository