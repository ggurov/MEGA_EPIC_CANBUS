# Configuration Guide

Complete guide for configuring MEGA_EPIC_CANBUS firmware.

## Quick Configuration

### 1. Set ECU CAN ID

Edit `mega_epic_canbus.ino`:

```cpp
#define ECU_CAN_ID 1  // Change to match your ECU's CAN ID (0-15)
```

**Important:** This must match the CAN ID configured in your epicEFI ECU firmware.

### 2. Configure Variable Hashes

Variable hashes identify variables in the ECU. Get these from your ECU's `variables.json` file.

#### Analog Inputs (16 channels)

Find variable names for analog inputs in `variables.json`, then:

```cpp
const int32_t VAR_HASH_ANALOG[16] = {
    /* A0 */ 0x12345678,  // Example hash from variables.json
    /* A1 */ 0x23456789,
    /* A2 */ 0x34567890,
    // ... continue for all 16 channels
};
```

**Variable naming convention:** Typically something like `analogIn0`, `analogIn1`, etc.

#### Digital Inputs (15-bit bitfield)

Single variable hash for all 15 digital inputs:

```cpp
const int32_t VAR_HASH_D20_D34 = 0x12345678;  // From variables.json
```

**Variable name:** Typically `digitalIn` or `buttonStates`

#### Digital Outputs (15-bit bitfield)

Single variable hash for all 15 digital outputs:

```cpp
const int32_t VAR_HASH_D35_D49 = 0x12345678;  // From variables.json
```

**Variable name:** Typically `digitalOut` or `relayStates`

**Important:** This must be a variable that the ECU can READ (not write-only).

#### PWM Outputs (14 channels)

Find variable names for each PWM channel:

```cpp
const int32_t VAR_HASH_PWM[14] = {
    /* D2  */ 0x12345678,
    /* D3  */ 0x23456789,
    /* D4  */ 0x34567890,
    /* D5  */ 0x45678901,
    /* D6  */ 0x56789012,
    /* D7  */ 0x67890123,
    /* D8  */ 0x78901234,
    /* D10 */ 0x89012345,
    /* D11 */ 0x90123456,
    /* D12 */ 0xA1234567,
    /* D13 */ 0xB2345678,
    /* D44 */ 0xC3456789,
    /* D45 */ 0xD4567890,
    /* D46 */ 0xE5678901,
};
```

**Variable naming convention:** Typically `pwmOut0`, `pwmOut1`, etc., or `motorSpeed0`, etc.

### 3. Get Variable Hashes

#### Method 1: From variables.json

1. Locate your ECU's `variables.json` file
2. Search for variable names (e.g., "analogIn0", "digitalOut")
3. Find the `hash` field (signed 32-bit integer)
4. Copy the hash value to firmware

**Example variables.json entry:**
```json
{
  "name": "analogIn0",
  "hash": -1234567890,
  "type": "float32"
}
```

**Firmware:**
```cpp
const int32_t VAR_HASH_ANALOG[0] = -1234567890;
```

#### Method 2: Calculate Hash

If you know the variable name, you can calculate the hash:

```cpp
// Hash algorithm (from EPIC protocol):
int32_t hash = 0;
for (char c : variableName) {
    hash = hash * 31 + (int32_t)c;
}
```

However, **it's recommended to use the hash from variables.json** to ensure correctness.

### 4. Verify Configuration

After configuring hashes, upload firmware and monitor Serial output:

```
WARNING: VAR_HASH_D35_D49 not set - digital outputs disabled
WARNING: VAR_HASH_PWM[] not set - PWM outputs disabled
```

If you see warnings, those features will be disabled until hashes are configured.

## Performance Configuration

### Transmission Rate

```cpp
#define TRANSMIT_INTERVAL_MS 25  // Milliseconds between TX cycles
```

- **Default:** 25ms (40 transmissions/second)
- **Range:** 10-100ms recommended
- **Lower = faster updates, higher CAN utilization**
- **Higher = slower updates, lower CAN utilization**

### Change Detection Threshold

```cpp
#define ANALOG_CHANGE_THRESHOLD 5.0  // ADC counts
```

- **Default:** 5.0 ADC counts (~24mV)
- **Purpose:** Transmit analog values only if change >= threshold
- **Lower = more sensitive, more CAN traffic**
- **Higher = less sensitive, less CAN traffic**

### Heartbeat Interval

```cpp
#define ANALOG_HEARTBEAT_INTERVAL_MS 1000  // Send all values every 1 second
```

- **Default:** 1000ms (1 second)
- **Purpose:** Send all analog values periodically, even if unchanged
- **Prevents stale data in ECU**
- **Set to 0 to disable heartbeat (transmit only on change)**

### Digital Input Change Detection

```cpp
#define DIGITAL_CHANGE_ONLY true  // Only transmit on state change
```

- **Default:** `true` (transmit only when state changes)
- **Purpose:** Reduces CAN traffic for buttons/switches
- **Set to `false` to transmit every cycle (not recommended)**

### Polling Intervals

```cpp
#define DIGITAL_OUTPUT_POLL_INTERVAL_MS 50   // Request digital outputs
#define PWM_POLL_INTERVAL_MS 100              // Request PWM outputs
```

- **Digital Outputs:** Requested every 50ms (20 requests/second)
- **PWM Outputs:** One channel per 100ms (round-robin, ~7 seconds for all 14)
- **Adjust based on required update rate**

## Error Handling Configuration

### ECU Communication Timeout

```cpp
#define ECU_COMM_TIMEOUT_MS 3000  // 3 seconds
```

- **Default:** 3000ms (3 seconds)
- **Behavior:** Enter safe mode if no ECU response for this duration
- **Safe Mode:** All outputs set to safe state (LOW for digital, 0% for PWM)

### CAN TX Retry

```cpp
#define CAN_TX_RETRY_COUNT 2       // Number of retries
#define CAN_TX_RETRY_DELAY_MS 10   // Delay between retries
```

- **Retry Count:** Attempt retransmission up to N times on failure
- **Retry Delay:** Milliseconds between retry attempts
- **Default:** 2 retries, 10ms delay

## Optional Features

### Interrupt Counters

```cpp
#define ENABLE_INTERRUPT_COUNTERS false
```

- **Enable:** Set to `true` to enable D18, D19 interrupt counters
- **Use Case:** Wheel speed sensors, frequency measurement
- **Variable Hashes:** Configure `VAR_HASH_COUNTER1` and `VAR_HASH_COUNTER2`

### ADC Calibration

```cpp
#define ENABLE_ADC_CALIBRATION false
```

- **Enable:** Set to `true` to enable per-channel calibration
- **Calibration:** Stored in EEPROM (offset + gain per channel)
- **Use Case:** Sensor scaling, offset correction

### EEPROM Configuration

```cpp
#define ENABLE_EEPROM_CONFIG true
```

- **Default:** `true` (enabled)
- **Stores:** ECU_CAN_ID, ADC calibration data
- **Persists:** Configuration survives reboots

## Testing Features

### Test Mode

```cpp
#define ENABLE_TEST_MODE false
```

- **Enable:** Set to `true` for automatic I/O test patterns
- **Behavior:** Cycles digital outputs ON/OFF, PWM through duty cycles
- **Use:** Hardware validation without ECU connection

### Serial Commands

```cpp
#define ENABLE_SERIAL_COMMANDS false
```

- **Enable:** Set to `true` for interactive serial commands
- **Commands:** STATS, TESTANALOG, TESTDIGITAL, TESTOUTPUTS, TESTPWM, HELP
- **Use:** Debugging and hardware testing

### Performance Statistics

```cpp
#define PERFORMANCE_STATS_INTERVAL_MS 10000  // Every 10 seconds (0 = disabled)
```

- **Enable:** Set to non-zero interval (milliseconds)
- **Output:** CAN frame rates, error rates, communication status
- **Default:** 10000ms (10 seconds)
- **Disable:** Set to 0

## Advanced Configuration

### SPI Chip Select Pin

If your CAN shield uses a different CS pin:

```cpp
#define SPI_CS_PIN 9  // Change if shield uses different pin
```

### Counter Reporting Interval

```cpp
#define COUNTER_REPORT_INTERVAL_MS 100  // Report counters every 100ms
```

- **Adjust** based on required frequency resolution
- **Lower = higher resolution, more CAN traffic**

### EEPROM Address Mapping

Advanced: Adjust EEPROM storage addresses if needed:

```cpp
#define EEPROM_ADDR_MAGIC       0   // Magic number (4 bytes)
#define EEPROM_ADDR_ECU_CAN_ID  4   // ECU_CAN_ID (1 byte)
#define EEPROM_ADDR_CALIBRATION 8   // ADC calibration (32 bytes)
```

**Note:** Changing these requires EEPROM clearing or migration.

## Configuration Validation

### Serial Output

On startup, firmware prints configuration status:

```
=== Feature Status ===
Interrupt Counters: DISABLED
ADC Calibration: DISABLED
EEPROM Config: ENABLED
Test Mode: DISABLED
=====================
```

### Hash Validation

Firmware checks for configured hashes:

```
WARNING: VAR_HASH_D35_D49 not set - digital outputs disabled
WARNING: VAR_HASH_PWM[] not set - PWM outputs disabled
```

**Fix:** Configure hashes from `variables.json`

### EEPROM Validation

If EEPROM config is enabled:

```
Configuration loaded from EEPROM
```

or

```
No valid EEPROM configuration found (using defaults)
```

## Configuration Examples

### Basic Setup (Analog + Digital Inputs Only)

```cpp
#define ECU_CAN_ID 1

// Configure analog inputs
const int32_t VAR_HASH_ANALOG[16] = { /* ... */ };

// Configure digital inputs
const int32_t VAR_HASH_D20_D34 = /* ... */;

// Leave outputs unconfigured (they'll be disabled)
```

### Full I/O Setup

```cpp
#define ECU_CAN_ID 1

// Configure all I/O
const int32_t VAR_HASH_ANALOG[16] = { /* ... */ };
const int32_t VAR_HASH_D20_D34 = /* ... */;
const int32_t VAR_HASH_D35_D49 = /* ... */;
const int32_t VAR_HASH_PWM[14] = { /* ... */ };
```

### High-Performance Setup

```cpp
#define TRANSMIT_INTERVAL_MS 10           // Faster updates
#define ANALOG_CHANGE_THRESHOLD 2.0       // More sensitive
#define DIGITAL_OUTPUT_POLL_INTERVAL_MS 25  // Faster polling
#define PWM_POLL_INTERVAL_MS 50           // Faster PWM updates
```

### Low-Bandwidth Setup

```cpp
#define TRANSMIT_INTERVAL_MS 50           // Slower updates
#define ANALOG_CHANGE_THRESHOLD 10.0      // Less sensitive
#define ANALOG_HEARTBEAT_INTERVAL_MS 5000 // Less frequent heartbeat
```

## Troubleshooting Configuration

### Problem: ECU not receiving data

- Verify `ECU_CAN_ID` matches ECU firmware
- Check variable hashes are correct (from variables.json)
- Monitor CAN bus with analyzer to verify frames are transmitted

### Problem: Outputs not responding

- Verify variable hash is configured correctly
- Check ECU is sending variable_response frames
- Monitor serial output for communication errors
- Verify ECU variable is readable (not write-only)

### Problem: Wrong variable values

- Verify variable hashes match ECU's variables.json
- Check byte order (firmware uses big-endian, correct)
- Verify data types match (float32 for most variables)

## See Also

- `PIN_ASSIGNMENT.md` - Pin map and wiring
- `TROUBLESHOOTING.md` - Common issues
- `TECHNICAL.md` - Protocol details

