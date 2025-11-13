# Seeed Studio CAN-BUS Shield v2.0 - Configuration Guide

**This branch is specifically configured for Seeed Studio CAN-BUS Shield v2.0**

The firmware is pre-configured with CS pin D10 (Seeed Studio default). No changes needed!

## Shield Information

### Seeed Studio CAN-BUS Shield v2.0
- **CS Pin:** D10 (pre-configured in this branch)
- **Library:** mcp_can (by Longan Labs - compatible with Seeed Studio shield)
- **Product Page:** https://www.seeedstudio.com/CAN-BUS-Shield-V2.html
- **Wiki:** https://wiki.seeedstudio.com/CAN-BUS_Shield_V2.0/
- **Status:** ✅ Fully configured and ready to use

### Shield Features
- **9-pin D-sub connector** - OBD-II and CAN standard pinouts
- **Screw terminals** - Easy connection for CAN_H, CAN_L, GND
- **Grove connectors** - I2C and UART interfaces
- **MCP2515 chip** - CAN controller
- **MCP2551 transceiver** - CAN bus transceiver

## Quick Identification

### Seeed Studio Shield v2.0 Features:
- CS pin on D10
- 9-pin D-sub connector on board
- OBD-II and CAN standard pinouts
- Grove connectors (I2C and UART)
- Screw terminals for CAN_H/CAN_L
- Professional industrial appearance

## No Configuration Needed!

This branch is already configured for Seeed Studio CAN-BUS Shield v2.0:

```cpp
// In mega_epic_canbus.ino:
#define SPI_CS_PIN  10  // Seeed Studio CAN-BUS Shield v2.0 default
```

**Just plug it in and go!**

## Library Installation

### Install mcp_can Library

1. **Arduino IDE** → **Tools** → **Manage Libraries...**
2. **Search:** "mcp_can"
3. **Install:** "mcp_can" by Longan Labs
4. ✅ Works perfectly with Seeed Studio shield

**Note:** The mcp_can library by Longan Labs is compatible with Seeed Studio shields and works great!

## Wiring Connections

### Arduino Mega2560 to Seeed Studio Shield

```
Arduino Mega2560    Seeed Studio CAN-BUS Shield v2.0
-----------------------------------------------------
D10 (SS)            CS (Chip Select) - SPI CS
D11 (MOSI)          SI (SPI In) - MOSI
D12 (MISO)          SO (SPI Out) - MISO
D13 (SCK)           SCK (SPI Clock)
GND                 GND (Common Ground)
5V                  VCC (Power Supply)
```

### CAN Bus Connection

The Seeed Studio shield has **screw terminals** for easy connection:

**Terminal Connections:**
- **CAN_H** → Connect to CAN bus high line
- **CAN_L** → Connect to CAN bus low line
- **GND** → Connect to CAN bus ground

**9-pin D-sub Connector:**
- The shield also has a 9-pin D-sub connector
- Supports OBD-II and CAN standard pinouts
- Use this for standard automotive connections

## Installation Steps

1. **Install Shield:**
   - Align pins carefully
   - Push shield down firmly onto Arduino Mega2560
   - Verify shield is properly seated

2. **Connect CAN Bus:**
   - Use screw terminals: CAN_H, CAN_L, GND
   - Or use 9-pin D-sub connector (OBD-II/CAN standard)
   - Install 120Ω termination resistor at ECU end

3. **Power On:**
   - Connect 7-12V DC power supply to Arduino
   - Shield gets power through Arduino

4. **Upload Firmware:**
   - CS pin is already set to D10 (no changes needed)
   - Upload and verify "CAN BUS OK!" message

## Verification

After installation, verify the configuration:

1. **Upload firmware** (CS pin already configured for D10)
2. **Open Serial Monitor** at 115200 baud
3. **Check for "CAN BUS OK!"** message
4. **If you see "CAN BUS FAIL!"**:
   - Verify shield is properly seated
   - Check SPI connections
   - Verify shield power supply
   - Check CS pin is set to 10 (should be pre-configured)

## Pin Usage Notes

### D10 (CS Pin)
- Used for CAN bus chip select
- SPI SS pin conflict is handled automatically by library
- No issues with using D10 for CAN

### Other SPI Pins
- **D11:** SPI MOSI (shared with other SPI devices)
- **D12:** SPI MISO (shared)
- **D13:** SPI SCK (shared)

### Grove Connectors
- **I2C Grove:** Available on shield (not used by firmware)
- **UART Grove:** Available on shield (not used by firmware)

## Troubleshooting

### Problem: "CAN BUS FAIL!" Message

**Solution:**
1. ✅ Verify shield is properly seated (no bent pins)
2. ✅ Check CS pin is set to 10 (already configured in this branch)
3. ✅ Verify mcp_can library is installed
4. ✅ Check power supply to shield (should get power from Arduino 5V)
5. ✅ Verify SPI connections

### Problem: Shield Not Detected

**Solution:**
1. Check shield seating (press down firmly)
2. Verify all pins are making contact
3. Check for bent pins
4. Try reseating shield

### Problem: CAN Bus Communication Issues

**Solution:**
1. Verify CAN_H and CAN_L connections (check screw terminals)
2. Check termination resistors (120Ω at each end of bus)
3. Verify common ground connection
4. Check CAN bus wiring (twisted pair recommended)
5. Verify ECU is powered and transmitting

## Additional Notes

- This firmware is optimized for Seeed Studio CAN-BUS Shield v2.0
- All features work identically to other MCP2515 shields
- CS pin D10 is standard for Seeed Studio shields
- Shield works great with mcp_can library by Longan Labs

## Switching to Different Shield

If you need to use a different shield (e.g., Longan Labs with D9):
- Switch to `main` branch (configured for Longan Labs)
- Or manually change CS pin in firmware

## See Also

- **INSTALLATION.md** - Complete installation guide
- **PIN_ASSIGNMENT.md** - Pin assignments and wiring
- **WIRING_DIAGRAMS.md** - Detailed wiring diagrams
- **TROUBLESHOOTING.md** - Comprehensive troubleshooting guide
