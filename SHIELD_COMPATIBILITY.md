# CAN Shield Compatibility Guide

This firmware supports MCP2515-based CAN bus shields. Different manufacturers use different CS (Chip Select) pins.

## Supported Shields

### Longan Labs CAN Bus Shield (Default Configuration)
- **CS Pin:** D9
- **Library:** mcp_can (by Longan Labs)
- **Product Link:** https://www.longan-labs.cc/1030016.html
- **Status:** ✅ Fully compatible (default configuration)
- **Configuration:** No changes needed

### Seeed Studio CAN-BUS Shield v2.0
- **CS Pin:** D10 (NOT D9)
- **Library:** Can use mcp_can library or Seeed Studio library
- **Product Link:** https://www.seeedstudio.com/CAN-BUS-Shield-V2.html
- **Status:** ✅ Compatible (requires CS pin change)
- **Configuration:** See setup instructions below

### Other MCP2515-Based Shields
- **CS Pin:** Varies (check shield documentation)
- **Library:** mcp_can or shield-specific library
- **Status:** ⚠️ May require CS pin configuration and library compatibility check

## Quick Identification

### Longan Labs Shield Features:
- CS pin typically on D9
- Uses mcp_can library
- May have on-board termination resistor switch

### Seeed Studio Shield v2.0 Features:
- CS pin on D10
- 9-pin D-sub connector
- OBD-II and CAN standard pinouts
- Grove connectors (I2C and UART)
- Screw terminals for CAN_H/CAN_L

## Configuration for Seeed Studio CAN-BUS Shield v2.0

### Step 1: Change CS Pin

Edit `mega_epic_canbus.ino`:

```cpp
// Change this line:
#define SPI_CS_PIN  9

// To this:
#define SPI_CS_PIN  10  // Seeed Studio CAN-BUS Shield v2.0
```

### Step 2: Install Library

You can use either:

**Option A: mcp_can library (Recommended)**
1. Arduino IDE → Tools → Manage Libraries
2. Search "mcp_can"
3. Install "mcp_can" by Longan Labs
4. ✅ Works with Seeed Studio shield

**Option B: Seeed Studio Library**
1. Arduino IDE → Tools → Manage Libraries
2. Search "CAN_BUS"
3. Install "CAN_BUS" by Seeed Studio
4. ⚠️ Requires code modifications (different API)

**We recommend Option A** (mcp_can library) as it requires only the CS pin change.

### Step 3: Verify Wiring

Seeed Studio Shield v2.0 Connections:
```
Arduino Mega2560    Seeed Studio Shield
─────────────────────────────────────────
D10 (SS)            CS (Chip Select)
D11 (MOSI)          SI (SPI In)
D12 (MISO)          SO (SPI Out)
D13 (SCK)           SCK (SPI Clock)
GND                 GND
5V                  VCC
```

### Step 4: CAN Bus Connection

Seeed Studio Shield has screw terminals:
- **CAN_H:** Connect to CAN bus high line
- **CAN_L:** Connect to CAN bus low line
- **GND:** Connect to CAN bus ground

The shield also has a 9-pin D-sub connector with OBD-II and CAN standard pinouts.

## CS Pin Configuration Table

| Shield Model | CS Pin | Default Config | Change Needed |
|--------------|--------|----------------|---------------|
| Longan Labs CAN Bus Shield | D9 | ✅ Yes | None |
| Seeed Studio CAN-BUS Shield v2.0 | D10 | ❌ No | Change to D10 |
| Other MCP2515 shields | Varies | ⚠️ Check docs | Change as needed |

## Verification

After changing the CS pin, verify the configuration:

1. **Upload firmware** with updated CS pin
2. **Open Serial Monitor** at 115200 baud
3. **Check for "CAN BUS OK!"** message
4. **If you see "CAN BUS FAIL!"**:
   - Verify CS pin setting matches your shield
   - Check shield is properly seated
   - Verify SPI connections
   - Check shield power supply

## Pin Conflicts

### D10 Usage

**Longan Labs Shield (D9 CS):**
- D10 is available for other uses (SPI SS is not used for CAN)

**Seeed Studio Shield (D10 CS):**
- D10 is used for CAN CS pin
- Still works fine (SPI SS pin conflict is handled by library)

### Other Pins

Both shields use:
- **D11:** SPI MOSI (shared)
- **D12:** SPI MISO (shared)
- **D13:** SPI SCK (shared)
- **GND:** Common ground
- **5V:** Power supply

## Troubleshooting

### Problem: "CAN BUS FAIL!" with Seeed Studio Shield

**Solution:**
1. Verify CS pin is set to 10 (not 9)
2. Check shield is properly seated
3. Verify mcp_can library is installed
4. Check power supply to shield
5. Verify SPI connections

### Problem: Shield Not Detected

**Solution:**
1. Check shield documentation for CS pin
2. Verify CS pin in firmware matches shield
3. Check shield seating (no bent pins)
4. Verify SPI connections with multimeter

### Problem: Library Conflicts

**Solution:**
- Use mcp_can library by Longan Labs (works with both shields)
- If using Seeed Studio library, you'll need code modifications
- Only install ONE CAN library to avoid conflicts

## Additional Notes

- Both shields use the same MCP2515 chip, so functionality is identical
- The only difference is the CS pin selection
- All other firmware features work the same regardless of shield brand
- CAN bus protocol and wiring are identical for both shields

## Need Help?

- **Longan Labs Shield:** Check product documentation
- **Seeed Studio Shield:** Check Seeed Studio wiki
- **Firmware Issues:** See TROUBLESHOOTING.md
- **General Setup:** See INSTALLATION.md

