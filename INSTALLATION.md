# Installation Instructions

Step-by-step guide for installing and setting up MEGA_EPIC_CANBUS firmware.

## Prerequisites

### Hardware Required

1. **Arduino Mega2560** (or compatible board)
   - Must be genuine Arduino or compatible clone
   - Verify board works (upload Blink example first)

2. **MCP_CAN Shield** (MCP2515-based CAN controller)
   - **Supported Shields:**
     - **Longan Labs CAN Bus Shield** (default, CS pin D9) ✅ Recommended
     - **Seeed Studio CAN-BUS Shield v2.0** (CS pin D10) ✅ Compatible
     - Other MCP2515-based shields (may require CS pin configuration)
   - Verify shield has MCP2515 chip
   - **If using Seeed Studio shield:** See `SHIELD_COMPATIBILITY.md` for setup

3. **CAN Bus Cables**
   - CAN_H, CAN_L, GND wires
   - Twisted pair recommended for CAN_H/CAN_L
   - Proper length for your application

4. **Termination Resistors**
   - Two 120Ω resistors (one for each end of bus)
   - Can be on shield, on ECU, or external

5. **Power Supply**
   - 7-12V DC regulated power supply
   - Barrel jack connector or VIN pin access
   - Adequate current capacity (1-2A recommended)

### Software Required

1. **Arduino IDE**
   - Version 1.8.x or 2.x recommended
   - Download from: https://www.arduino.cc/en/software

2. **Arduino Mega Board Support**
   - Included in Arduino IDE by default
   - Verify "Arduino Mega or Mega 2560" appears in Board menu

3. **MCP_CAN Library**
   - Install via Arduino Library Manager
   - Search "mcp_can" and install "mcp_can" by Longan Labs

## Step 1: Install Arduino IDE

1. Download Arduino IDE from https://www.arduino.cc/en/software
2. Install following standard procedure for your OS
3. Launch Arduino IDE
4. Verify installation: File → Examples → 01.Basics → Blink should open

## Step 2: Install MCP_CAN Library

1. Open Arduino IDE
2. Go to **Tools → Manage Libraries...**
3. Search for "mcp_can"
4. Find "mcp_can" by Longan Labs
5. Click **Install** button
6. Wait for installation to complete
7. Verify: File → Examples should show "mcp_can" examples

## Step 3: Download Firmware

1. Download or clone MEGA_EPIC_CANBUS repository
2. Extract if needed
3. Locate `mega_epic_canbus.ino` file

## Step 4: Configure Firmware

1. Open `mega_epic_canbus.ino` in Arduino IDE
2. Edit basic configuration:

```cpp
#define ECU_CAN_ID 1  // Change to match your ECU's CAN ID (0-15)
```

3. Get variable hashes from your ECU's `variables.json`:
   - Locate ECU firmware's `variables.json` file
   - Find variable names (e.g., "analogIn0", "digitalOut")
   - Copy hash values to firmware

4. See `CONFIGURATION.md` for detailed configuration instructions

## Step 5: Connect Hardware

### Arduino + Shield Connection

```
BEFORE:
┌──────────────┐
│   Arduino    │
│   Mega2560   │
│              │
│  [Pins]      │
│  [Pins]      │
└──────────────┘

    │ Align pins carefully
    │ Push down firmly
    │
    ▼

AFTER:
┌──────────────┐
│   Arduino    │
│   Mega2560   │
│              │
│  ┌─────────┐│
│  │  Shield ││
│  │  MCP2515││
│  │  CAN    ││
│  └─────────┘│
│              │
└──────────────┘
```

**Steps:**
1. **Power off** Arduino and ECU
2. Align shield pins with Arduino header
3. Press down firmly until shield sits flush
4. Verify shield is properly seated (no gaps)
5. Check for bent pins

### CAN Bus Wiring

```
CAN BUS CONNECTION:
┌──────────────┐          ┌──────────────┐
│ epicEFI ECU  │          │  Arduino     │
│              │          │  Mega2560    │
│  CAN_H ──────┼──────────┼──► CAN_H     │
│  CAN_L ──────┼──────────┼──► CAN_L     │
│  GND   ──────┼──────────┼──► GND       │
│              │          │              │
│ [120Ω]       │          │              │
│ Terminator   │          │              │
└──────────────┘          └──────────────┘
```

**Steps:**
1. **CAN_H**: Connect shield CAN_H to CAN bus high line
2. **CAN_L**: Connect shield CAN_L to CAN bus low line
3. **GND**: Connect shield GND to CAN bus ground
4. **Termination**: Install 120Ω resistor at Arduino end (if not on shield)
5. **Termination**: Install 120Ω resistor at ECU end

**Important:**
- CAN_H and CAN_L must be twisted pair (or close together)
- Only TWO termination resistors total (one at each end)
- Common ground is required

### I/O Wiring

**See `WIRING_DIAGRAMS.md` for complete printable ASCII wiring diagrams**

**Quick Reference:**

1. **Analog Inputs** (A0-A15):
```
Sensor Signal ──────────► Arduino A0-A15
Sensor GND    ──────────► Arduino GND
Sensor VCC    ──────────► Arduino 5V (or external)
```

2. **Digital Inputs** (D20-D34):
```
Button Terminal 1 ──────► Arduino D20-D34
Button Terminal 2 ──────► Arduino GND (LOW = active)
```

3. **Digital Outputs** (D35-D49):
```
Arduino D35-D49 ──► [Driver] ──► [Load/Relay]
```

4. **PWM Outputs** (D2-D8, D10-D13, D44-D46):
```
Arduino PWM Pin ──► [Motor Driver] ──► [Motor/Actuator]
```

See `PIN_ASSIGNMENT.md` and `WIRING_DIAGRAMS.md` for detailed wiring diagrams.

## Step 6: Connect USB

1. Connect USB cable to Arduino Mega2560
2. Connect other end to computer
3. Verify Arduino appears in Device Manager (Windows) or `/dev/tty*` (Linux/Mac)

## Step 7: Configure Arduino IDE

1. **Select Board:**
   - Tools → Board → Arduino AVR Boards → Arduino Mega or Mega 2560

2. **Select Port:**
   - Tools → Port → [Your Arduino COM Port]
   - Windows: COM3, COM4, etc.
   - Linux/Mac: /dev/ttyACM0, /dev/ttyUSB0, etc.

3. **Verify SPI CS Pin:**
   - Check `#define SPI_CS_PIN 9` matches your shield
   - Some shields use D10 - check shield documentation

## Step 8: Upload Firmware

1. Click **Verify** button (checkmark icon) to compile
   - Wait for "Done compiling" message
   - Fix any compilation errors if present

2. Click **Upload** button (arrow icon) to upload
   - Wait for "Done uploading" message
   - Arduino will reset automatically

3. **If upload fails:**
   - Check USB cable connection
   - Verify correct COM port selected
   - Try pressing reset button during upload
   - Check for driver issues (Windows may need CH340/FTDI drivers)

## Step 9: Verify Installation

### Check Serial Output

1. Open Serial Monitor: Tools → Serial Monitor
2. Set baud rate to **115200**
3. Press Arduino reset button (or power cycle)
4. Look for startup messages:

```
CAN BUS OK!
Pin initialization complete
=== Feature Status ===
```

### Test Communication

1. **Verify CAN Bus:**
   - Serial should show "CAN BUS OK!"
   - No "CAN BUS FAIL!" errors

2. **Verify ECU Communication:**
   - Wait for ECU response (if ECU is connected)
   - Serial may show "WARNING: No ECU response received yet" initially
   - Should see "ECU communication restored" when ECU responds

3. **Check Performance:**
   - Enable `PERFORMANCE_STATS_INTERVAL_MS 10000` for statistics
   - Monitor CAN TX/RX frame rates

### Test I/O (Optional)

1. **Enable Serial Commands:**
   ```cpp
   #define ENABLE_SERIAL_COMMANDS true
   ```

2. **Upload and Open Serial Monitor:**
   - Send `HELP` command
   - Send `TESTANALOG` to test analog inputs
   - Send `TESTDIGITAL` to test digital inputs
   - Send `TESTOUTPUTS` to test digital outputs
   - Send `TESTPWM` to test PWM outputs

## Step 10: Production Setup

### Disable Debug Features

For production use, disable debug/test features:

```cpp
#define ENABLE_TEST_MODE false
#define ENABLE_SERIAL_COMMANDS false
#define PERFORMANCE_STATS_INTERVAL_MS 0  // Disable periodic stats
```

### Final Verification

1. **Power Cycle:** Power off/on to verify persistence
2. **Long Test:** Run for extended period (hours) to verify stability
3. **Monitor:** Check for error messages or communication issues
4. **Verify Outputs:** Confirm outputs respond correctly to ECU commands

## Troubleshooting Installation

### Compilation Errors

**Error: "mcp_can.h: No such file or directory"**
- Solution: Install MCP_CAN library (see Step 2)

**Error: "Board not found"**
- Solution: Install Arduino AVR Boards package (Tools → Board → Boards Manager)

**Other compilation errors:**
- Check Arduino IDE version (1.8.x or 2.x recommended)
- Verify all required libraries are installed
- Check for syntax errors in code

### Upload Errors

**Error: "Couldn't find a Board on the selected port"**
- Solution: Check USB cable, try different port, verify board type selected

**Error: "avrdude: stk500_getsync() timeout"**
- Solution: Press reset button on Arduino just before clicking Upload
- Try different USB cable or port

**Error: "Permission denied" (Linux/Mac)**
- Solution: Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Log out and log back in

### Hardware Issues

**Shield not detected:**
- Check shield is properly seated
- Verify SPI connections
- Check shield power LED (if present)

**CAN bus not initializing:**
- Check SPI connections
- Verify MCP2515 power supply
- Check CS pin configuration

**See `TROUBLESHOOTING.md` for detailed troubleshooting guide**

## Next Steps

1. **Configure Variable Hashes:**
   - Get hashes from ECU's variables.json
   - Update firmware with correct hashes
   - See `CONFIGURATION.md`

2. **Test Hardware:**
   - Use serial commands to test I/O
   - Verify CAN bus communication
   - Check ECU communication

3. **Integration:**
   - Connect to ECU
   - Verify bidirectional communication
   - Test end-to-end functionality

4. **Documentation:**
   - Read `README.md` for overview
   - Read `CONFIGURATION.md` for configuration
   - Read `PIN_ASSIGNMENT.md` for wiring

## Uninstallation

To remove firmware:

1. Upload blank sketch (File → Examples → 01.Basics → BareMinimum)
2. Or restore factory bootloader (if needed)

**Note:** Firmware is stored in Arduino flash memory and is overwritten when new firmware is uploaded.

## See Also

- `README.md` - Project overview and quick start
- `CONFIGURATION.md` - Configuration guide
- `PIN_ASSIGNMENT.md` - Hardware wiring
- `TROUBLESHOOTING.md` - Problem solving

