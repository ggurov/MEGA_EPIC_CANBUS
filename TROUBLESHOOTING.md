# Troubleshooting Guide

Common issues, solutions, and debugging procedures for MEGA_EPIC_CANBUS.

## Quick Diagnostics

### Serial Monitor Output

Always check Serial Monitor (115200 baud) for diagnostic messages:

- **"CAN BUS OK!"** - CAN initialization successful
- **"ECU communication restored"** - ECU communication re-established
- **"WARNING: ECU communication lost"** - ECU communication timeout
- **Performance statistics** - Shows CAN frame rates and errors

## Common Issues

### Compilation Errors

**Symptoms:**
- Compilation fails with SPI library errors
- Error messages like: `error: request for member 'spcr' in 'settings'`
- Error messages like: `error: request for member 'spsr' in 'settings'`
- Error path shows: `arduino\hardware\avr\1.8.6\libraries\SPI\src/SPI.h`

**Possible Causes & Solutions:**

1. **Outdated Arduino AVR Board Package**
   - ✅ The error shows Arduino AVR hardware package version 1.8.6 (very old)
   - ✅ Update to latest Arduino AVR board package (1.8.13+ recommended)
   - ✅ **Solution:** Tools → Board → Boards Manager → Search "Arduino AVR Boards" → Update to latest version

2. **Library Compatibility**
   - ✅ MCP_CAN library may be incompatible with old SPI library
   - ✅ Try updating MCP_CAN library to latest version
   - ✅ Library Manager → Search "mcp_can" → Update if available

3. **Arduino IDE Version**
   - ✅ Older Arduino IDE versions may have compatibility issues
   - ✅ Update to Arduino IDE 1.8.19+ or Arduino IDE 2.x
   - ✅ Download from: https://www.arduino.cc/en/software

**Debug Steps:**
1. Check Arduino IDE version (Help → About Arduino)
2. Check Board Package version:
   - Tools → Board → Boards Manager
   - Search "Arduino AVR Boards"
   - Note current version (should be 1.8.13 or newer)
3. Update if needed:
   - Click "Update" or "Install" button next to Arduino AVR Boards
   - Wait for update to complete
   - Restart Arduino IDE
4. Try compiling again

**Quick Fix Steps:**
1. Open Arduino IDE
2. Go to **Tools → Board → Boards Manager...**
3. Search for **"Arduino AVR Boards"**
4. If version is 1.8.6 or older, click **"Update"** or **"Install"** button
5. Wait for update to complete (may take a few minutes)
6. Restart Arduino IDE
7. Try compiling again

**Alternative:** If update doesn't work, try:
- Uninstall Arduino AVR Boards package
- Reinstall latest version
- Or switch to Arduino IDE 2.x (more modern)

### CAN Bus Not Initializing

**Symptoms:**
- Serial output shows "CAN BUS FAIL!" repeatedly
- No CAN frames transmitted
- Arduino appears stuck in initialization loop

**Possible Causes & Solutions:**

1. **SPI Connections**
   - ✅ Check MOSI (D11), MISO (D12), SCK (D13), CS (D9) connections
   - ✅ Verify shield is properly seated on Arduino
   - ✅ Try reseating shield connections

2. **MCP2515 Power Supply**
   - ✅ Verify 5V power to MCP2515 chip
   - ✅ Check shield power LED (if present)
   - ✅ Measure voltage at VCC pin of MCP2515 (should be ~5V)

3. **Chip Select Pin**
   - ✅ Verify CS pin matches shield configuration
   - ✅ Check `#define SPI_CS_PIN` matches shield (default: D9)
   - ✅ Some shields use D10 - check shield documentation

4. **Library Issues**
   - ✅ Verify MCP_CAN library is installed (Library Manager)
   - ✅ Check library version compatibility
   - ✅ Try reinstalling library

5. **Hardware Fault**
   - ✅ Try different MCP_CAN shield
   - ✅ Try different Arduino board
   - ✅ Check for damaged components

**Debug Steps:**
1. Enable serial output and monitor for error messages
2. Check SPI connections with multimeter
3. Verify shield power supply voltage
4. Try known-good hardware if available

### No ECU Communication

**Symptoms:**
- Serial output shows "WARNING: No ECU response received yet"
- No variable_response frames received
- Safe mode activates after 3 seconds

**Possible Causes & Solutions:**

1. **ECU CAN ID Mismatch**
   - ✅ Verify `ECU_CAN_ID` in firmware matches ECU's CAN ID
   - ✅ Check ECU firmware configuration
   - ✅ Monitor CAN bus with analyzer to see actual ECU CAN IDs

2. **CAN Bus Wiring**
   - ✅ Check CAN_H, CAN_L, GND connections
   - ✅ Verify CAN_H and CAN_L are not swapped
   - ✅ Ensure common ground between Arduino and ECU

3. **CAN Bus Termination**
   - ✅ Verify 120Ω termination resistors at both ends of bus
   - ✅ Only TWO termination resistors total (one at each end)
   - ✅ Remove extra termination resistors if present

4. **ECU Not Transmitting**
   - ✅ Verify ECU is powered and running
   - ✅ Check ECU firmware is loaded and active
   - ✅ Verify ECU CAN bus is enabled in ECU firmware

5. **CAN Bus Baud Rate**
   - ✅ Verify firmware uses `CAN_500KBPS` (default)
   - ✅ Ensure ECU also uses 500kbps
   - ✅ Check CAN bus shield supports 500kbps

6. **Variable Hash Mismatch**
   - ✅ Verify variable hashes match ECU's variables.json
   - ✅ Check ECU has variables configured for I/O expansion
   - ✅ Monitor CAN bus to see if ECU responds to requests

**Debug Steps:**
1. Enable `PERFORMANCE_STATS_INTERVAL_MS` to monitor frame rates
2. Use CAN bus analyzer to verify frames are being transmitted
3. Check for variable_response frames from ECU (0x720 + ECU_CAN_ID)
4. Verify ECU CAN ID with ECU configuration tool

### Analog Inputs Reading Incorrectly

**Symptoms:**
- ADC values don't match expected voltage
- All inputs read same value
- Values jump around erratically
- Values stuck at 1023 or 0

**Possible Causes & Solutions:**

1. **Voltage Range Exceeded**
   - ✅ Verify sensor output is 0-5V (maximum for Arduino ADC)
   - ✅ Use voltage divider for 0-12V sensors (e.g., 10kΩ + 7kΩ)
   - ✅ Check sensor specifications

2. **Floating Inputs**
   - ✅ Floating inputs read ~5V due to internal pullup
   - ✅ Connect sensor to input (or ground input if not used)
   - ✅ Use external pullup/pulldown if needed

3. **Ground Issues**
   - ✅ Ensure common ground between Arduino and sensor
   - ✅ Check ground connections are solid
   - ✅ Avoid ground loops (use single ground point)

4. **ADC Calibration Needed**
   - ✅ Enable `ENABLE_ADC_CALIBRATION`
   - ✅ Calibrate each channel with known voltage sources
   - ✅ Store calibration in EEPROM

5. **Noise/Signal Integrity**
   - ✅ Use shielded cable for analog sensors
   - ✅ Add filtering capacitor (100nF) near sensor
   - ✅ Keep analog wires away from digital/PWM wires
   - ✅ Use twisted pair for sensor wiring

6. **Sensor Power Supply**
   - ✅ Verify sensor has stable power supply
   - ✅ Check sensor output when disconnected from Arduino
   - ✅ Measure sensor voltage with multimeter

**Debug Steps:**
1. Use `TESTANALOG` serial command to read all channels
2. Compare ADC readings with multimeter voltage measurements
3. Test with known voltage source (e.g., 3.3V from Arduino)
4. Enable ADC calibration if consistent offset/gain error

### Digital Inputs Not Working

**Symptoms:**
- Inputs always read HIGH or LOW
- State changes not detected
- Multiple inputs show same state

**Possible Causes & Solutions:**

1. **Wiring Issues**
   - ✅ Verify connections to D20-D34
   - ✅ Check switch/button is connected correctly
   - ✅ Ensure switch connects to GND when pressed (LOW = active)

2. **Logic Level**
   - ✅ Remember: LOW (grounded) = 1 (active), HIGH (pullup) = 0 (inactive)
   - ✅ Verify switch grounds input when pressed
   - ✅ Check for switch bounce (may need debouncing in ECU)

3. **Pullup Issues**
   - ✅ Internal pullup is enabled (input should read HIGH when floating)
   - ✅ Verify pullup is working (float input, should read HIGH)
   - ✅ Check if external pullup conflicts with internal

4. **Short Circuit**
   - ✅ Check for short between pins
   - ✅ Verify no solder bridges on PCB
   - ✅ Test with multimeter (continuity check)

5. **Pin Conflict**
   - ✅ D20-D21 also used for I2C (if I2C not used, OK)
   - ✅ Verify pins aren't used elsewhere in code

**Debug Steps:**
1. Use `TESTDIGITAL` serial command to read all inputs
2. Test with jumper wire (connect to GND manually)
3. Verify pullup works (float input, should read HIGH)
4. Check bitfield value matches expected pattern

### Digital Outputs Not Responding

**Symptoms:**
- Outputs don't change state
- All outputs stuck at one state
- Outputs work but don't follow ECU commands

**Possible Causes & Solutions:**

1. **Variable Hash Not Configured**
   - ✅ Check Serial output for "VAR_HASH_D35_D49 not set" warning
   - ✅ Configure `VAR_HASH_D35_D49` from variables.json
   - ✅ Verify hash matches ECU's variable

2. **ECU Not Sending variable_response**
   - ✅ Verify ECU is sending variable_response frames
   - ✅ Check CAN bus communication (see "No ECU Communication")
   - ✅ Monitor CAN bus to see variable_response frames

3. **ECU Variable Not Readable**
   - ✅ Verify ECU variable is read/write (not write-only)
   - ✅ Check ECU variable type matches (float32 bitfield)
   - ✅ Verify ECU firmware has I/O expansion enabled

4. **Communication Lost (Safe Mode)**
   - ✅ Check Serial output for "ECU communication lost" message
   - ✅ Fix ECU communication (see troubleshooting above)
   - ✅ Outputs disabled in safe mode (by design)

5. **Load Issues**
   - ✅ Verify output can drive connected load
   - ✅ Check current doesn't exceed 40mA per pin
   - ✅ Use transistor/mosfet driver for high-current loads

**Debug Steps:**
1. Use `TESTOUTPUTS` serial command to test outputs directly
2. Monitor CAN bus for variable_response frames (0x720 + ECU_CAN_ID)
3. Check `PERFORMANCE_STATS` for RX frame count
4. Verify variable hash in ECU's variables.json

### PWM Outputs Not Working

**Symptoms:**
- PWM outputs stuck at 0% or 100%
- Outputs don't respond to ECU commands
- Output voltage doesn't vary

**Possible Causes & Solutions:**

1. **Variable Hashes Not Configured**
   - ✅ Check Serial output for "VAR_HASH_PWM[] not set" warning
   - ✅ Configure `VAR_HASH_PWM[14]` array from variables.json
   - ✅ Verify all 14 hashes are configured

2. **ECU Not Sending PWM Values**
   - ✅ Verify ECU is sending variable_response for PWM variables
   - ✅ Check ECU has PWM output variables configured
   - ✅ Monitor CAN bus for variable_response frames

3. **Communication Lost**
   - ✅ Check for safe mode activation (all PWM = 0%)
   - ✅ Fix ECU communication issues
   - ✅ Verify communication timeout settings

4. **Load Issues**
   - ✅ Verify PWM signal reaches load (check with scope/multimeter)
   - ✅ Use PWM driver circuit (transistor/mosfet) for motor loads
   - ✅ Check PWM frequency compatibility with load

5. **PWM Frequency**
   - ✅ D2-D13: ~490Hz (may be too slow for some applications)
   - ✅ D44-D46: ~980Hz (faster, may be better for motors)
   - ✅ Consider hardware PWM modules if higher frequency needed

**Debug Steps:**
1. Use `TESTPWM` serial command to test PWM outputs directly
2. Monitor CAN bus for variable_response frames with PWM variable hashes
3. Check Serial output for PWM request/response messages
4. Measure PWM output with oscilloscope or PWM meter

### CAN Bus Errors/Frame Loss

**Symptoms:**
- High error rate in performance statistics
- Missing frames or data corruption
- Intermittent communication

**Possible Causes & Solutions:**

1. **Bus Termination**
   - ✅ Verify exactly 2 termination resistors (120Ω each)
   - ✅ One at Arduino end, one at ECU end
   - ✅ Remove extra termination resistors

2. **Bus Length**
   - ✅ Maximum recommended: ~40 meters at 500kbps
   - ✅ Use shorter bus for better reliability
   - ✅ Consider lower baud rate for longer buses

3. **Cable Quality**
   - ✅ Use twisted pair cable for CAN_H/CAN_L
   - ✅ Avoid single wires or ribbon cable
   - ✅ Keep CAN bus wires away from high-voltage/noisy wires

4. **Ground Issues**
   - ✅ Ensure solid ground connection
   - ✅ Avoid ground loops (single ground point)
   - ✅ Use thick ground wire if long distance

5. **Bus Loading**
   - ✅ Too many nodes can cause issues
   - ✅ Check total bus capacitance
   - ✅ Verify bus impedance (~60Ω with termination)

6. **EMI/Noise**
   - ✅ Shield CAN bus cable if in noisy environment
   - ✅ Keep away from motors, ignition coils, alternators
   - ✅ Use ferrite cores if needed

**Debug Steps:**
1. Enable `PERFORMANCE_STATS_INTERVAL_MS` to monitor error rate
2. Use CAN bus analyzer to see actual error frames
3. Check CAN_H and CAN_L differential voltage with scope
4. Measure bus termination resistance (should be ~60Ω)

## Hardware Verification Checklist

Use this checklist to verify hardware setup:

### Arduino & Shield
- [ ] Arduino Mega2560 powered (7-12V via barrel jack)
- [ ] MCP_CAN shield properly seated
- [ ] Shield power LED on (if present)
- [ ] Serial Monitor shows "CAN BUS OK!"

### CAN Bus Wiring
- [ ] CAN_H connected (not swapped with CAN_L)
- [ ] CAN_L connected
- [ ] GND connected (common ground)
- [ ] 120Ω termination resistor at Arduino end
- [ ] 120Ω termination resistor at ECU end
- [ ] Only 2 termination resistors total

### Input Wiring
- [ ] Analog sensors connected to A0-A15 (0-5V)
- [ ] Digital inputs connected to D20-D34 (LOW = active)
- [ ] Ground connections for all sensors
- [ ] No floating inputs (connect or ground unused)

### Output Wiring
- [ ] Digital outputs connected to D35-D49
- [ ] PWM outputs connected to D2-D8, D10-D13, D44-D46
- [ ] Loads properly connected (with drivers if needed)
- [ ] Flyback diodes for inductive loads (relays)

### ECU Configuration
- [ ] ECU powered and running
- [ ] ECU CAN ID matches firmware `ECU_CAN_ID`
- [ ] ECU firmware has I/O expansion enabled
- [ ] Variable hashes configured in firmware

## Serial Debug Output Interpretation

### Normal Operation

```
CAN BUS OK!
Pin initialization complete
Performance optimization: Change detection enabled
=== Feature Status ===
...
```

**Indicates:** Firmware initialized successfully

### Warnings

```
WARNING: VAR_HASH_D35_D49 not set - digital outputs disabled
```

**Action:** Configure variable hash from variables.json

```
WARNING: No ECU response received yet (check CAN bus connection)
```

**Action:** Check CAN bus wiring and ECU communication

### Errors

```
CAN BUS FAIL!
```

**Action:** Check SPI connections, power supply, shield seating

```
WARNING: ECU communication lost - entering safe mode
```

**Action:** Fix ECU communication (see troubleshooting above)

### Performance Statistics

```
=== Performance Statistics ===
CAN TX Success: 1200 (10.0 frames/sec)
CAN RX Frames: 600 (5.0 frames/sec)
CAN RX Errors: 2 (0.33%)
```

**Interpretation:**
- TX rate: Number of frames transmitted per second
- RX rate: Number of frames received per second
- Error rate: Percentage of received frames with errors
- **Good:** Error rate < 1%, RX rate > 0

## Advanced Debugging

### CAN Bus Analyzer

Use CAN bus analyzer to monitor traffic:

1. **Verify Frame Transmission:**
   - Look for variable_set frames (0x780 + ECU_CAN_ID)
   - Check variable_request frames (0x700 + ECU_CAN_ID)
   - Verify frame data matches expected format

2. **Verify ECU Response:**
   - Look for variable_response frames (0x720 + ECU_CAN_ID)
   - Check function_response frames (0x760 + ECU_CAN_ID)
   - Verify frame timing (should be < 100ms response time)

3. **Check for Errors:**
   - Error frames indicate bus problems
   - High error count = wiring/termination issues
   - Missing frames = ECU not responding

### Serial Command Testing

Enable serial commands for interactive testing:

```cpp
#define ENABLE_SERIAL_COMMANDS true
```

Commands:
- `STATS` - Check performance and communication status
- `TESTANALOG` - Verify analog inputs
- `TESTDIGITAL` - Verify digital inputs
- `TESTOUTPUTS` - Test digital outputs directly
- `TESTPWM` - Test PWM outputs directly

### Test Mode

Enable test mode for automatic I/O patterns:

```cpp
#define ENABLE_TEST_MODE true
```

Useful for hardware validation without ECU connection.

## Still Having Issues?

1. **Verify Configuration:**
   - Check all variable hashes are configured correctly
   - Verify ECU_CAN_ID matches ECU
   - Review configuration guide

2. **Check Hardware:**
   - Use hardware verification checklist
   - Test with known-good components if available
   - Verify wiring with multimeter

3. **Monitor Traffic:**
   - Use CAN bus analyzer to see actual traffic
   - Check Serial output for error messages
   - Enable performance statistics

4. **Document:**
   - Note exact symptoms
   - Capture Serial output
   - Record CAN bus analyzer traces (if available)

## See Also

- `CONFIGURATION.md` - Configuration guide
- `PIN_ASSIGNMENT.md` - Wiring diagrams
- `TECHNICAL.md` - Protocol details

