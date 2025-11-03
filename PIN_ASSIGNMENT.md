# Pin Assignment Reference

Complete pin assignment and wiring guide for MEGA_EPIC_CANBUS firmware.

## Pin Map Overview

| Function | Pins | Count | Notes |
|----------|------|-------|-------|
| Analog Inputs | A0-A15 | 16 | 10-bit ADC, 0-5V |
| Digital Inputs | D20-D34 | 15 | Pullup-enabled, LOW=active |
| Digital Outputs | D35-D49 | 15 | HIGH/LOW, ECU-controlled |
| PWM Outputs | D2-D8, D10-D13, D44-D46 | 14 | 8-bit PWM, 0-100% |
| Interrupt Counters | D18, D19 | 2 | Optional, for wheel speed |
| SPI (CAN) | D9, D10-D13 | 5 | MCP2515 CAN controller |
| UART | D0-D1 | 2 | Serial Monitor (USB) |

## Detailed Pin Assignments

### Analog Inputs (A0-A15)

**Purpose:** Sensor inputs (voltage, temperature, pressure, etc.)

- **A0-A15**: 10-bit ADC (0-1023), 0-5V input range
- **Pullup:** Internal pullup enabled (affects floating inputs)
- **Resolution:** ~4.9mV per ADC count
- **Variable Hash:** Per-channel (16 hashes total)

**Wiring:**
- Sensor output → Arduino analog pin (A0-A15)
- Sensor GND → Arduino GND
- Sensor VCC → 5V or external supply (check sensor specs)

**Notes:**
- Maximum input voltage: 5V (exceeding may damage ADC)
- Floating inputs will read ~5V (pullup active)
- For 0-12V sensors, use voltage divider (e.g., 10kΩ + 7kΩ)
- Enable ADC calibration for sensor scaling

### Digital Inputs (D20-D34)

**Purpose:** Button/switch inputs, digital sensors

- **D20-D34**: 15 pins, packed into 15-bit bitfield
- **Logic:** LOW (grounded) = 1 (active), HIGH (pullup) = 0 (inactive)
- **Pullup:** Internal pullup enabled
- **Bitfield:** Bit 0 = D20, Bit 14 = D34
- **Variable Hash:** Single hash for 15-bit bitfield

**Wiring:**
- Button/switch → Arduino digital pin (D20-D34)
- Button/switch other terminal → GND (for active LOW)
- Floating = HIGH (pullup active, inactive state)

**Notes:**
- Use LOW-active switches (normally HIGH via pullup, LOW when pressed)
- For HIGH-active switches, invert in ECU logic
- Bitfield transmitted as float32 (cast to uint16_t on ECU side)

### Digital Outputs (D35-D49)

**Purpose:** Relay control, LED drivers, solenoid control

- **D35-D49**: 15 pins, controlled via ECU variable_response
- **Logic:** HIGH = ON, LOW = OFF
- **Safe State:** LOW (all outputs LOW on communication loss)
- **Bitfield:** 15-bit bitfield from ECU (Bit 0 = D35, Bit 14 = D49)
- **Polling:** Requested every 50ms from ECU

**Wiring:**
- Relay coil → Arduino digital pin (D35-D49) → GND
- Use flyback diode for inductive loads (relays, solenoids)
- For high-current loads, use transistor/mosfet driver

**Notes:**
- Maximum current per pin: 40mA (use drivers for higher current)
- All outputs initialized to LOW (safe state)
- Outputs disabled in safe mode (communication loss)

### PWM Outputs (D2-D8, D10-D13, D44-D46)

**Purpose:** Motor speed control, valve position, LED dimming

**Pins:** D2, D3, D4, D5, D6, D7, D8, D10, D11, D12, D13, D44, D45, D46 (14 total)

- **Resolution:** 8-bit (0-255), 0-100% duty cycle
- **Frequency:** ~490Hz (D2-D13), ~980Hz (D44-D46)
- **Safe State:** 0% duty cycle (all outputs off)
- **Polling:** Round-robin, one channel per 100ms
- **Variable Hash:** Per-channel (14 hashes total)

**Wiring:**
- PWM signal → Driver/Controller → Load
- Common use cases:
  - Motor speed control (via motor driver)
  - Valve position (via servo/actuator)
  - LED dimming (via MOSFET/transistor)

**Notes:**
- D9 skipped (used for CAN SPI CS)
- Maximum current per pin: 40mA (use drivers)
- For motor control, use dedicated motor driver IC

### Interrupt Counters (D18, D19) - Optional

**Purpose:** Wheel speed sensors, frequency measurement

- **D18 (INT3)**: Counter 1 input (FALLING edge trigger)
- **D19 (INT2)**: Counter 2 input (FALLING edge trigger)
- **Pullup:** Internal pullup enabled
- **Enable:** Set `ENABLE_INTERRUPT_COUNTERS = true`
- **Reporting:** Delta counts transmitted every 100ms

**Wiring:**
- Sensor output → Arduino digital pin (D18 or D19)
- Sensor GND → Arduino GND
- Sensor VCC → 5V or 12V (check sensor specs)

**Notes:**
- Also UART1 pins (TX1/RX1) - conflicts if UART1 needed
- Useful for wheel speed sensors (hall effect, magnetic)
- Frequency = (delta_counts / 0.1 seconds)

### SPI Interface (CAN Bus)

**Purpose:** Communication with MCP2515 CAN controller

- **D9**: SPI Chip Select (CS) - MCP2515 CS pin
- **D10 (SS)**: SPI Slave Select (not used for CAN)
- **D11 (MOSI)**: SPI Master Out Slave In
- **D12 (MISO)**: SPI Master In Slave Out
- **D13 (SCK)**: SPI Serial Clock

**Wiring (MCP_CAN Shield):**
- Shield typically plugs directly into Arduino
- Verify CS pin matches shield (D9 default)
- CAN_H and CAN_L connect to CAN bus

**CAN Bus Wiring:**
- CAN_H → CAN bus high line
- CAN_L → CAN bus low line
- GND → CAN bus ground
- 120Ω termination resistor at each end of bus

**Notes:**
- CAN bus requires termination (120Ω resistors)
- Maximum bus length: ~40 meters (at 500kbps)
- Use twisted pair cable for CAN_H/CAN_L

### Reserved/Used Pins

**Avoid Using:**
- **D0-D1**: UART0 (USB Serial - used by Serial Monitor)
- **D9**: SPI CS for MCP2515 (unless shield uses different pin)
- **D10 (SS)**: SPI Slave Select (leave as OUTPUT, HIGH)
- **D18-D19**: UART1 (also used for interrupt counters if enabled)
- **D20-D21**: I2C SDA/SCL (also used for digital inputs)

## Wiring Diagrams

### Typical Sensor Connection

```
Sensor (0-5V)
    |
    +--[Signal]----> A0 (Analog Input)
    |
    +--[GND]-------> Arduino GND
    |
    +--[VCC]-------> 5V or External Supply
```

### Digital Input (Button)

```
Button/Switch
    |
    +--[One Terminal]----> D20-D34
    |
    +--[Other Terminal]--> GND (LOW = active)
```

### Digital Output (Relay)

```
Arduino D35-D49 ----[Series Resistor]----> Transistor Base
                                              |
                                              +--[Collector]---> Relay Coil ---> 12V
                                              |
                                              +--[Emitter]-----> GND
                                              |
                                              [Flyback Diode across relay coil]
```

### PWM Output (Motor)

```
Arduino PWM Pin ----> Motor Driver (PWM Input)
                         |
                         +---> Motor Terminal 1
                         +---> Motor Terminal 2
                         +---> Power Supply (VCC, GND)
```

### CAN Bus Connection

```
Arduino Mega + MCP_CAN Shield
    |
    +--[CAN_H]---> CAN Bus High (twisted pair)
    |
    +--[CAN_L]---> CAN Bus Low (twisted pair)
    |
    +--[GND]-----> CAN Bus Ground

CAN Bus Termination:
    [120Ω Resistor] at Arduino end
    [120Ω Resistor] at ECU end
```

## Power Supply

### Arduino Power Options

1. **USB Power** (5V via USB cable)
   - Limited current (~500mA)
   - Sufficient for testing only
   - Not recommended for production

2. **Barrel Jack** (7-12V DC)
   - Recommended for production
   - Use regulated 9V or 12V supply
   - Ensure adequate current capacity

3. **VIN Pin** (7-12V DC)
   - Alternative to barrel jack
   - Same requirements as barrel jack

### Power Requirements

- **Arduino Mega2560:** ~200-300mA typical
- **MCP_CAN Shield:** ~50-100mA
- **I/O Loads:** Depends on connected devices
- **Total:** Plan for 1-2A supply for safety margin

## Pin Usage Summary Table

| Pin | Function | I/O | Notes |
|-----|----------|-----|-------|
| D0 | UART0 RX | I | USB Serial (avoid) |
| D1 | UART0 TX | O | USB Serial (avoid) |
| D2-D8 | PWM Output | O | 8-bit PWM |
| D9 | SPI CS | O | MCP2515 Chip Select |
| D10 | SPI SS | O | Leave HIGH |
| D11 | SPI MOSI | O | SPI data out |
| D12 | SPI MISO | I | SPI data in |
| D13 | SPI SCK | O | SPI clock |
| D13 | PWM Output | O | Also PWM-capable |
| D18 | INT3/Counter1 | I | Optional, also UART1 TX |
| D19 | INT2/Counter2 | I | Optional, also UART1 RX |
| D20-D34 | Digital Input | I | Pullup, LOW=active |
| D35-D49 | Digital Output | O | ECU-controlled |
| D44-D46 | PWM Output | O | 8-bit PWM |
| A0-A15 | Analog Input | I | 10-bit ADC, 0-5V |

## Best Practices

1. **Grounding:** Ensure common ground between Arduino and all sensors/loads
2. **Power Supply:** Use regulated supply with adequate current capacity
3. **CAN Termination:** Always include 120Ω termination resistors
4. **Isolation:** Consider opto-isolation for high-voltage loads
5. **Protection:** Use flyback diodes for inductive loads (relays, solenoids)
6. **Current Limits:** Respect 40mA per-pin limit (use drivers for higher current)
7. **Voltage Limits:** Keep analog inputs within 0-5V range
8. **Wire Gauge:** Use appropriate gauge for current requirements
9. **Shielding:** Use shielded cables for analog sensors if needed
10. **Documentation:** Document your wiring for future reference

## See Also

- `CONFIGURATION.md` - Variable hash configuration
- `TROUBLESHOOTING.md` - Hardware debugging
- `TECHNICAL.md` - Technical details

