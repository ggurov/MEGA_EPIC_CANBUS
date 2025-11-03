# Wiring Diagrams - Line Printable Format

Complete ASCII art wiring diagrams for MEGA_EPIC_CANBUS installation.

## Table of Contents

1. [System Overview](#system-overview)
2. [Arduino Pin Layout](#arduino-pin-layout)
3. [Analog Input Wiring](#analog-input-wiring)
4. [Digital Input Wiring](#digital-input-wiring)
5. [Digital Output Wiring](#digital-output-wiring)
6. [PWM Output Wiring](#pwm-output-wiring)
7. [CAN Bus Connection](#can-bus-connection)
8. [Power Supply](#power-supply)
9. [Complete System Diagram](#complete-system-diagram)

## System Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                    MEGA_EPIC_CANBUS SYSTEM                      │
└─────────────────────────────────────────────────────────────────┘

┌──────────────┐         CAN Bus         ┌──────────────┐
│              │  ───────────────────────  │              │
│ epicEFI ECU  │◄─────── (500 kbps) ─────►│ Arduino Mega │
│              │                           │  + MCP_CAN   │
│              │                           │    Shield     │
└──────┬───────┘                           └──────┬───────┘
       │                                          │
       │                                          │
       │                          ┌───────────────┴───────────────┐
       │                          │                               │
       │                          ▼                               ▼
       │                   ┌─────────────┐                ┌─────────────┐
       │                   │   Sensors   │                │  Actuators  │
       │                   │ (Analog/    │                │  (Digital/  │
       │                   │  Digital)   │                │    PWM)     │
       │                   └─────────────┘                └─────────────┘
       │
       └─────────────────────────────────────────────────────────┘
                        Common Ground (GND)
```

## Arduino Pin Layout

```
                    ARDUINO MEGA2560 - TOP VIEW
┌──────────────────────────────────────────────────────────────────┐
│                                                                  │
│  USB Connector                                            Power  │
│      │                                                      │     │
│      ▼                                                      ▼     │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                                                          │   │
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐   │   │
│  │  │ 0││ 1││ 2││ 3││ 4││ 5││ 6││ 7││ 8││ 9││10││11│   │   │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘   │   │
│  │                                                          │   │
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐   │   │
│  │  │12││13││  ││  ││  ││  ││  ││  ││  ││20││21││22│   │   │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘   │   │
│  │                                                          │   │
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐   │   │
│  │  │23││24││25││26││27││28││29││30││31││32││33││34│   │   │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘   │   │
│  │                                                          │   │
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐   │   │
│  │  │35││36││37││38││39││40││41││42││43││44││45││46│   │   │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘   │   │
│  │                                                          │   │
│  │  ┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐┌──┐   │   │
│  │  │47││48││49││50││51││52││53││  ││  ││  ││  ││  │   │   │
│  │  └──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘└──┘   │   │
│  │                                                          │   │
│  │  ┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐      │   │
│  │  │ A0 ││ A1 ││ A2 ││ A3 ││ A4 ││ A5 ││ A6 ││ A7 │      │   │
│  │  └────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘      │   │
│  │                                                          │   │
│  │  ┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐┌────┐      │   │
│  │  │ A8 ││ A9 ││ A10││ A11││ A12││ A13││ A14││ A15│      │   │
│  │  └────┘└────┘└────┘└────┘└────┘└────┘└────┘└────┘      │   │
│  │                                                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘

PIN FUNCTION LEGEND:
D0-D1:   UART0 (USB Serial - avoid)
D2-D8:   PWM Outputs
D10:     SPI CS (MCP2515 CAN controller) - Seeed Studio CAN-BUS Shield v2.0
D10-D13: PWM Outputs + SPI
D18-D19: Interrupt Counters (optional) or UART1
D20-D34: Digital Inputs
D35-D49: Digital Outputs
D44-D46: PWM Outputs
A0-A15:  Analog Inputs
```

## Analog Input Wiring

### Single Analog Sensor Connection

```
ANALOG SENSOR (0-5V)
┌─────────────┐
│             │
│  Sensor     │
│  Module     │
│             │
└───┬────┬────┘
    │    │
    │    │
    │    └────────────┐
    │                 │
    │ Signal          │ VCC
    │ (0-5V)          │ (+5V or External)
    │                 │
    ▼                 ▼
  ┌───────────────────────┐
  │   Arduino Mega2560    │
  │                       │
  │  ┌────┐               │
  │  │ A0 │───────────────┼──► Signal Input
  │  └────┘               │
  │                       │
  │  ┌────┐               │
  │  │ GND├───────────────┼──► Ground
  │  └────┘               │
  │                       │
  │  ┌────┐               │
  │  │ 5V ├───────────────┼──► VCC (if needed)
  │  └────┘               │
  │                       │
  └───────────────────────┘
```

### Multiple Analog Sensors (Daisy Chain Ground)

```
SENSOR 1              SENSOR 2              SENSOR 3
┌──────────┐         ┌──────────┐         ┌──────────┐
│ Signal ──┼────────► A0       │         │ Signal ──┼────────► A2
│ GND   ───┼──┐      │ VCC      │         │ GND   ───┼──┐      │
└──────────┘  │      └──────────┘         └──────────┘  │      │
              │                                      │      │
SENSOR 2      │                                      │      │
┌──────────┐  │                                      │      │
│ Signal ──┼──┼────────► A1                         │      │
│ GND   ───┼──┼──┐      │ VCC                        │      │
└──────────┘  │  │      └─────────────┐              │      │
              │  │                    │              │      │
              │  │                    │              │      │
              │  │                    │              │      │
              │  └────────────────────┼──────────────┼──────┘
              │                       │              │
              │                  ┌─────▼───────┐     │
              │                  │   Arduino   │     │
              │                  │   Mega2560  │     │
              │                  │             │     │
              │                  │  ┌────┐     │     │
              │                  │  │GND ├─────┼─────┘
              │                  │  └────┘     │
              │                  │             │
              │                  │  ┌────┐     │
              │                  │  │ 5V ├─────┼────► All VCC connections
              │                  │  └────┘     │      (if using Arduino 5V)
              │                  │             │
              │                  └─────────────┘
              │
              └──► Common Ground Bus
```

### Voltage Divider for 0-12V Sensors

```
12V SENSOR OUTPUT
     │
     ├───[10kΩ Resistor]───┐
     │                      │
     │                      │
     │                      ▼
     │                 ┌─────────┐
     │                 │  Arduino│
     │                 │   A0    │───► 0-5V (scaled)
     │                 └─────────┘
     │                      │
     │                      │
     └──────────────────────┼───[7kΩ Resistor]───► GND
                            │
                            │
                     ┌──────▼──────┐
                     │   Arduino   │
                     │    GND     │
                     └─────────────┘

FORMULA:
  V_arduino = V_sensor × (R2 / (R1 + R2))
  Example: 12V × (7k / (10k + 7k)) = 12V × 0.412 = 4.94V

CALIBRATION:
  Enable ENABLE_ADC_CALIBRATION and set gain/offset
```

## Digital Input Wiring

### Single Button/Switch Connection

```
PUSHBUTTON/SWITCH
    ┌─────┐
    │     │
    │  ●  │  (Normally open button)
    │     │
    └──┬──┘
       │
       │ Terminal 1
       │
       │
       ▼
  ┌─────────────┐
  │  Arduino    │
  │  Mega2560   │
  │             │
  │  ┌──────┐   │
  │  │ D20  │───┼──► Signal Input
  │  └──────┘   │    (Internal pullup)
  │             │    HIGH = inactive
  │             │    LOW = active (pressed)
  │  ┌──────┐   │
  │  │ GND  │───┼──► Ground
  │  └──────┘   │
  │             │
  └─────────────┘
       ▲
       │
       │ Terminal 2
       │
       └────────────► Connect to GND
```

### Multiple Digital Inputs

```
BUTTON 1        BUTTON 2        BUTTON 3
┌─────┐         ┌─────┐         ┌─────┐
│  ●  │         │  ●  │         │  ●  │
└──┬──┘         └──┬──┘         └──┬──┘
   │                │                │
   │                │                │
   │ Terminal 1     │ Terminal 1     │ Terminal 1
   │                │                │
   ▼                ▼                ▼
┌──────┐        ┌──────┐        ┌──────┐
│ D20  │        │ D21  │        │ D22  │
└──────┘        └──────┘        └──────┘
   │                │                │
   │                │                │
   │ Terminal 2     │ Terminal 2     │ Terminal 2
   │                │                │
   └────┬───────────┼───────────────┘
        │           │
        │           │
        │           └─────────────┐
        │                         │
        │                    ┌─────▼────┐
        │                    │  Arduino │
        │                    │  Mega2560│
        │                    │          │
        │                    │  ┌────┐ │
        └────────────────────┼──►│GND │ │
                             │  └────┘ │
                             │          │
                             └──────────┘
                    Common Ground Bus

LOGIC:
  Button NOT pressed: D20-D34 = HIGH (pullup active) = 0 (inactive)
  Button pressed:      D20-D34 = LOW (grounded)      = 1 (active)

BITFIELD:
  Bit 0  = D20
  Bit 1  = D21
  Bit 2  = D22
  ...
  Bit 14 = D34
```

## Digital Output Wiring

### Single Relay Connection

```
ARDUINO MEGA2560
┌──────────────┐
│              │
│  ┌──────┐    │
│  │ D35  │────┼──► To Transistor Base
│  └──────┘    │    (via current-limiting resistor)
│              │
│  ┌──────┐    │
│  │ GND  │────┼──► Common Ground
│  └──────┘    │
│              │
└──────────────┘
        │
        │
        ▼
   ┌─────────┐
   │  10kΩ  │  Current-limiting resistor
   │Resistor │
   └────┬────┘
        │
        │
        ▼
   ┌─────────────────┐
   │   NPN Transistor │
   │   (2N2222, etc.) │
   │                  │
   │  Base ◄──────────┼── From Arduino D35
   │  Collector       │
   │  Emitter         │
   └──┬───────────┬───┘
      │           │
      │           │
      │           └───► To GND
      │
      ▼
   ┌─────────────┐
   │   Relay     │
   │   Coil      │
   │             │
   │  Terminal 1 │
   │  Terminal 2 │
   └──┬───────┬──┘
      │       │
      │       │
      │       └──────┐
      │              │
      │              ▼
      │         ┌─────────┐
      │         │  12V    │
      │         │  Power  │
      │         │  Supply │
      │         └─────────┘
      │
      │
      ▼
   ┌─────┐      ┌─────────┐
   │Flyback│     │   Load   │
   │Diode │     │  (Motor, │
   │(1N4007)    │  Light,  │
   └─────┘      │  etc.)   │
                └─────────┘

FLYBACK DIODE:
  Connect across relay coil (cathode to +12V side)
  Prevents voltage spike when relay turns off
```

### Multiple Digital Outputs

```
ARDUINO MEGA2560
┌─────────────────────────────────┐
│                                 │
│  ┌──────┐  ┌──────┐  ┌──────┐ │
│  │ D35  │  │ D36  │  │ D37  │ │
│  └──┬───┘  └──┬───┘  └──┬───┘ │
│     │        │        │       │
│     │        │        │       │
│     ▼        ▼        ▼       │
│  [Resistor] [Resistor] [Resistor] │
│     │        │        │       │
│     ▼        ▼        ▼       │
│  [Transistor][Transistor][Transistor]│
│     │        │        │       │
│     ▼        ▼        ▼       │
│  [Relay 1]  [Relay 2] [Relay 3]│
│                                 │
└─────────────────────────────────┘

CONTROL:
  ECU sends bitfield via variable_response
  Bit 0 = D35, Bit 1 = D36, Bit 2 = D37, etc.

SAFE STATE:
  All outputs LOW when communication lost
```

## PWM Output Wiring

### Single Motor Connection

```
ARDUINO MEGA2560
┌──────────────┐
│              │
│  ┌──────┐    │
│  │ D2   │────┼──► PWM Signal (0-100% duty cycle)
│  └──────┘    │
│              │
│  ┌──────┐    │
│  │ GND  │────┼──► Ground
│  └──────┘    │
│              │
└──────────────┘
        │
        │
        ▼
   ┌──────────────────┐
   │  Motor Driver    │
   │  (L298N, etc.)  │
   │                  │
   │  PWM Input ◄─────┼── From Arduino D2
   │  Motor +         │
   │  Motor -         │
   │  Power +         │
   │  Power -         │
   └──┬────────────┬──┘
      │            │
      │            │
      ▼            ▼
   ┌─────┐      ┌─────┐
   │ 12V  │      │ GND │
   │Supply│      └─────┘
   └─────┘
      │
      │
      ▼
   ┌─────────┐
   │  Motor  │
   └─────────┘

PWM CHARACTERISTICS:
  Frequency: ~490Hz (D2-D13) or ~980Hz (D44-D46)
  Resolution: 8-bit (0-255 = 0-100%)
  Duty Cycle: 0% = motor stopped, 100% = motor full speed
```

### Multiple PWM Outputs

```
ARDUINO MEGA2560
┌──────────────────────────────────────────┐
│                                         │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐ │
│  │ D2   │ │ D3   │ │ D4   │ │ D5   │ │
│  └──┬───┘ └──┬───┘ └──┬───┘ └──┬───┘ │
│     │       │       │       │       │
│     │       │       │       │       │
│     ▼       ▼       ▼       ▼       │
│  [Driver] [Driver] [Driver] [Driver] │
│     │       │       │       │       │
│     ▼       ▼       ▼       ▼       │
│  [Motor1][Motor2][Valve1][Valve2]    │
│                                         │
└──────────────────────────────────────────┘

CONTROL:
  ECU sends duty cycle (0-100%) per channel
  Round-robin polling (one channel per 100ms)
```

## CAN Bus Connection

### CAN Bus Topology

```
                                CAN BUS
                                   │
        ┌──────────────────────────┼──────────────────────────┐
        │                          │                          │
        │                          │                          │
        ▼                          ▼                          ▼
┌──────────────┐          ┌──────────────┐          ┌──────────────┐
│              │          │              │          │              │
│ epicEFI ECU  │          │  Arduino     │          │ Other CAN     │
│              │          │  Mega2560    │          │ Devices       │
│              │          │  + MCP_CAN   │          │               │
│  CAN_H ──────┼──────────┼──► CAN_H     │          │ CAN_H ────────┤
│  CAN_L ──────┼──────────┼──► CAN_L     │          │ CAN_L ────────┤
│  GND   ──────┼──────────┼──► GND       │          │ GND   ────────┤
│              │          │              │          │               │
│ [120Ω]       │          │              │          │               │
│ Terminator   │          │              │          │ [120Ω]        │
│              │          │              │          │ Terminator    │
└──────────────┘          └──────────────┘          └──────────────┘
    (Node 1)                  (Node 2)                  (Node 3)

TERMINATION:
  Exactly TWO 120Ω resistors total
  One at each end of bus (not at every node)
  Creates 60Ω total impedance (120Ω || 120Ω)
```

### MCP_CAN Shield Connection

```
ARDUINO MEGA2560 TOP VIEW
┌────────────────────────────┐
│                            │
│  ┌──────────────────────┐ │
│  │  Arduino Pins        │ │
│  │                      │ │
│  │  D9  ────────────────┼─┼──► Not used (available)
│  │  D10 ────────────────┼─┼──► SPI CS (to MCP2515) - Seeed Studio shield
│  │  D11 ────────────────┼─┼──► SPI MOSI (to MCP2515)
│  │  D12 ────────────────┼─┼──► SPI MISO (from MCP2515)
│  │  D13 ────────────────┼─┼──► SPI SCK (to MCP2515)
│  │                      │ │
│  └──────────────────────┘ │
│                            │
└────────────────────────────┘
            │
            │ Shield plugs directly
            │ into Arduino header
            ▼
┌────────────────────────────┐
│   MCP_CAN SHIELD           │
│                            │
│  ┌──────────────────────┐ │
│  │  MCP2515 Chip        │ │
│  │                      │ │
│  │  CS  ◄────────────────┼── D10 (Seeed Studio shield)
│  │  SI  ◄────────────────┼── D11 (MOSI)
│  │  SO  ────────────────┼──► D12 (MISO)
│  │  SCK ◄────────────────┼── D13
│  │                      │ │
│  └──────────────────────┘ │
│                            │
│  ┌──────────────────────┐ │
│  │  CAN Transceiver      │ │
│  │  (MCP2551 or similar)│ │
│  │                      │ │
│  │  CAN_H ──────────────┼──► To CAN Bus High
│  │  CAN_L ──────────────┼──► To CAN Bus Low
│  │  GND   ──────────────┼──► To CAN Bus Ground
│  │                      │ │
│  └──────────────────────┘ │
│                            │
│  ┌──────────────────────┐ │
│  │  Termination Resistor │ │
│  │  (120Ω - if on shield)│ │
│  └──────────────────────┘ │
│                            │
└────────────────────────────┘
```

### CAN Bus Cable Wiring

```
TWISTED PAIR CABLE (Recommended)
┌─────────────────────────────────────────┐
│                                         │
│  CAN_H ────────────────────────         │
│         ╲         ╱                      │
│          ╲       ╱                       │
│           ╲     ╱                        │
│            ╲   ╱                         │
│             ╲ ╱                          │
│              ╳  (Twisted together)       │
│             ╱ ╲                          │
│            ╱   ╲                         │
│           ╱     ╲                        │
│          ╱       ╲                       │
│         ╱         ╲                       │
│  CAN_L ────────────────────────         │
│                                         │
│  GND ───────────────────────────────────│
│                                         │
└─────────────────────────────────────────┘

LENGTH:
  Maximum recommended: ~40 meters at 500 kbps
  Shorter = better signal integrity
  Use quality twisted pair cable
```

## Power Supply

### Power Supply Connection

```
POWER SUPPLY (7-12V DC, Regulated)
┌──────────────┐
│              │
│  +9V or +12V │
│  -0V (GND)   │
│              │
└──┬───────┬───┘
   │       │
   │       │
   │       └───► GND (common ground)
   │
   │
   ▼
┌──────────────────┐
│  Barrel Jack     │
│  (Arduino)       │
│                  │
│  VIN             │
│  GND             │
└──────┬───────────┘
       │
       │
       ▼
┌──────────────────┐
│  Arduino         │
│  Mega2560        │
│                  │
│  On-board        │
│  Regulator       │
│  (5V @ ~200mA)   │
│                  │
│  ┌────┐          │
│  │ 5V ├─────────┼──► To MCP_CAN Shield
│  └────┘          │
│                  │
│  ┌────┐          │
│  │GND ├──────────┼──► Common Ground
│  └────┘          │
│                  │
└──────────────────┘

ALTERNATIVE: VIN Pin
  Connect 7-12V directly to VIN pin (skip barrel jack)
  Same requirements apply

CURRENT REQUIREMENTS:
  Arduino Mega2560:     ~200-300mA
  MCP_CAN Shield:       ~50-100mA
  I/O Loads:            Variable
  Recommended Supply:   1-2A capacity
```

## Complete System Diagram

```
COMPLETE SYSTEM WIRING DIAGRAM
═══════════════════════════════════════════════════════════════════

                    POWER SUPPLY
                    ┌──────────┐
                    │ 9-12V DC │
                    │ Regulated│
                    └────┬─────┘
                         │
                         ▼
                ┌─────────────────┐
                │  Arduino Mega   │
                │  2560 + Shield  │
                │                 │
                │  ┌───────────┐  │
                │  │ MCP2515   │  │
                │  │ CAN Ctrl  │  │
                │  └──────┬───┘  │
                │         │      │
                │         ▼      │
                │  ┌───────────┐ │
                │  │ CAN       │ │
                │  │ Transceiver│
                │  └──────┬───┘ │
                │         │     │
                └─────────┼─────┘
                          │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
        │                 │                 │
        ▼                 ▼                 ▼
   ┌─────────┐      ┌─────────┐      ┌─────────┐
   │ CAN_H   │      │ CAN_L   │      │ GND     │
   └────┬────┘      └────┬────┘      └────┬────┘
        │                │                │
        │                │                │
        └────────────────┼────────────────┘
                         │
                         │ CAN Bus (500 kbps)
                         │
                         │
        ┌────────────────┼────────────────┐
        │                │                │
        ▼                ▼                ▼
   ┌─────────┐      ┌─────────┐      ┌─────────┐
   │ 120Ω    │      │  [Bus]   │      │ 120Ω    │
   │Term.    │      │          │      │Term.    │
   └─────────┘      └──────────┘      └─────────┘
        │                                 │
        │                                 │
        ▼                                 ▼
   ┌──────────┐                      ┌──────────┐
   │ epicEFI  │                      │ Other    │
   │  ECU     │                      │ CAN      │
   │          │                      │ Devices  │
   └──────────┘                      └──────────┘


I/O CONNECTIONS:

ANALOG INPUTS (A0-A15):
  Sensor1 ──► A0  Sensor2 ──► A1  ...  Sensor16 ──► A15
    │           │     │           │          │           │
    └─────┬─────┘     └─────┬─────┘          └─────┬─────┘
          │                 │                      │
          └─────────────────┼──────────────────────┘
                            │
                            ▼
                       ┌─────────┐
                       │ Common  │
                       │ Ground  │
                       └─────────┘

DIGITAL INPUTS (D20-D34):
  Button1 ──► D20  Button2 ──► D21  ...  Button15 ──► D34
    │          │     │          │            │          │
    │          │     │          │            │          │
    └──────────┼─────┼──────────┼────────────┼──────────┘
               │     │          │            │
               └─────┼──────────┼────────────┘
                     │          │
                     │          │
                     └──────────┘
                         │
                         ▼
                    ┌─────────┐
                    │ Common  │
                    │ Ground  │
                    └─────────┘

DIGITAL OUTPUTS (D35-D49):
  D35 ──► [Driver] ──► Relay1
  D36 ──► [Driver] ──► Relay2
  D37 ──► [Driver] ──► Relay3
  ...
  D49 ──► [Driver] ──► Relay15

PWM OUTPUTS (D2-D8, D10-D13, D44-D46):
  D2  ──► [Driver] ──► Motor1
  D3  ──► [Driver] ──► Motor2
  D4  ──► [Driver] ──► Valve1
  ...
  D46 ──► [Driver] ──► Actuator14

═══════════════════════════════════════════════════════════════════
```

## Assembly Steps (Visual Guide)

### Step 1: Install Shield

```
BEFORE:
┌──────────────┐
│   Arduino    │
│   Mega2560   │
│              │
│  [Pins]      │
│  [Pins]      │
│  [Pins]      │
└──────────────┘

    │
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

### Step 2: Connect CAN Bus

```
1. Connect CAN_H:
   Shield CAN_H ──────────────────► ECU CAN_H

2. Connect CAN_L:
   Shield CAN_L ──────────────────► ECU CAN_L

3. Connect Ground:
   Shield GND   ──────────────────► ECU GND

4. Install Termination:
   [120Ω] at Shield end (if not already on shield)
   [120Ω] at ECU end
```

### Step 3: Connect Sensors

```
For each analog sensor:
  Sensor Signal ──────────► Arduino A0-A15
  Sensor GND    ──────────► Arduino GND
  Sensor VCC    ──────────► Arduino 5V (or external)
```

### Step 4: Connect Outputs

```
For each digital output:
  Arduino D35-D49 ──► [Driver Circuit] ──► [Load]

For each PWM output:
  Arduino D2-D8, D10-D13, D44-D46 ──► [Driver] ──► [Motor/Actuator]
```

### Step 5: Power On

```
1. Connect Power Supply:
   [Power Supply] ──► [Arduino Barrel Jack]

2. Verify LEDs:
   - Power LED ON
   - CAN Shield LED ON (if present)

3. Check Serial Output:
   - "CAN BUS OK!" message
   - No error messages
```

## Notes for Printing

All diagrams use standard ASCII characters:
- `─` `│` `┌` `┐` `└` `┘` `├` `┤` `┬` `┴` `┼` for boxes and lines
- `►` `◄` for arrows
- `═` `║` for double lines
- Standard text characters for labels

These will print correctly on:
- Line printers
- Plain text printers
- ASCII-only displays
- Text editors
- Documentation systems

