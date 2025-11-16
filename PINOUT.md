## Arduino Mega2560 Pinout (Project Assignments and Capabilities)

**Notes**
- **PWM-capable pins:** D2–D13, D44–D46  
- **External interrupts:** INT0(D21), INT1(D20), INT2(D19), INT3(D18)  
- **SPI (hardware):** D50 (MISO), D51 (MOSI), D52 (SCK), D53 (SS)  
- **I2C (TWI):** D20 (SDA), D21 (SCL) — disabled in firmware, used as VSS interrupts  

### Digital Pins

| Pin  | Label     | Function / Assignment                         | Timer / IRQ          | Notes                                      |
|------|-----------|-----------------------------------------------|----------------------|--------------------------------------------|
| D0   | RX0       | UART0 RX (spare GPIO)                         |                      | Not used by firmware                       |
| D1   | TX0       | UART0 TX (spare GPIO)                         |                      | Not used by firmware                       |
| D2   |           | PWM `MEGA_EPIC_1_PWM_T3_D2`                   | Timer3               | Planned PWM output                         |
| D3   |           | PWM `MEGA_EPIC_1_PWM_T3_D3`                   | Timer3               | Planned PWM output                         |
| D4   |           | PWM-capable (Timer0)                          | Timer0               | ~1 kHz; avoid reconfiguring (Arduino time) |
| D5   |           | PWM `MEGA_EPIC_1_PWM_T3_D5`                   | Timer3               | Planned PWM output                         |
| D6   |           | PWM `MEGA_EPIC_1_PWM_T4_D6`                   | Timer4               | Planned PWM output                         |
| D7   |           | PWM `MEGA_EPIC_1_PWM_T4_D7`                   | Timer4               | Planned PWM output                         |
| D8   |           | PWM `MEGA_EPIC_1_PWM_T4_D8`                   | Timer4               | Planned PWM output                         |
| D9   |           | MCP_CAN CS                                    | Timer2               | Reserved for CAN shield CS                 |
| D10  |           | Spare PWM-capable GPIO                        | Timer2               | Available                                  |
| D11  |           | PWM `MEGA_EPIC_1_PWM_T1_D11`                  | Timer1               | Planned PWM output                         |
| D12  |           | PWM `MEGA_EPIC_1_PWM_T1_D12`                  | Timer1               | Planned PWM output                         |
| D13  | LED       | Onboard LED, PWM-capable                      | Timer0               | ~1 kHz; avoid reconfiguring (Arduino time) |
| D14  | TX3       | UART3 TX (spare GPIO)                         |                      | Available if UART3 unused                  |
| D15  | RX3       | UART3 RX (spare GPIO)                         |                      | Available if UART3 unused                  |
| D16  | TX2       | UART2 TX (spare GPIO)                         |                      | Available if UART2 unused                  |
| D17  | RX2       | UART2 RX (spare GPIO)                         |                      | Available if UART2 unused                  |
| D18  |           | VSS FrontLeft                                 | INT3                 | Falling edge, pullup enabled               |
| D19  |           | VSS FrontRight                                | INT2                 | Falling edge, pullup enabled               |
| D20  | SDA       | VSS RearLeft                                  | INT1 / I2C SDA       | Falling edge, pullup; I2C disabled         |
| D21  | SCL       | VSS RearRight                                 | INT0 / I2C SCL       | Falling edge, pullup; I2C disabled         |
| D22  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D23  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D24  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D25  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D26  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D27  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D28  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D29  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D30  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D31  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D32  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D33  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D34  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D35  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D36  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D37  |           | Digital input (button bitfield)               |                      | INPUT_PULLUP, inverted (LOW=1)             |
| D38  |           | Spare GPIO                                    |                      | Candidate future low-speed output          |
| D39  |           | Low-speed output `MEGA_EPIC_1_SLOW_D39`       |                      | Planned low-speed output                   |
| D40  |           | Low-speed output `MEGA_EPIC_1_SLOW_D40`       |                      | Planned low-speed output                   |
| D41  |           | Low-speed output `MEGA_EPIC_1_SLOW_D41`       |                      | Planned low-speed output                   |
| D42  |           | Low-speed output `MEGA_EPIC_1_SLOW_D42`       |                      | Planned low-speed output                   |
| D43  |           | Low-speed output `MEGA_EPIC_1_SLOW_D43`       |                      | Planned low-speed output                   |
| D44  |           | PWM `MEGA_EPIC_1_PWM_T5_D44`                  | Timer5               | Planned PWM output                         |
| D45  |           | PWM `MEGA_EPIC_1_PWM_T5_D45`                  | Timer5               | Planned PWM output                         |
| D46  |           | PWM `MEGA_EPIC_1_PWM_T5_D46`                  | Timer5               | Planned PWM output                         |
| D47  |           | Low-speed output `MEGA_EPIC_1_SLOW_D47`       |                      | Planned low-speed output                   |
| D48  |           | Low-speed output `MEGA_EPIC_1_SLOW_D48`       |                      | Planned low-speed output                   |
| D49  |           | Low-speed output `MEGA_EPIC_1_SLOW_D49`       |                      | Planned low-speed output                   |
| D50  | MISO      | SPI MISO                                      | SPI                  | Reserved for MCP_CAN                       |
| D51  | MOSI      | SPI MOSI                                      | SPI                  | Reserved for MCP_CAN                       |
| D52  | SCK       | SPI SCK                                       | SPI                  | Reserved for MCP_CAN                       |
| D53  | SS        | SPI SS / general SS                           | SPI                  | Reserved / general SPI SS                  |

### Analog Pins

| Pin  | Function / Assignment                      | Notes                        |
|------|--------------------------------------------|------------------------------|
| A0   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A1   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A2   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A3   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A4   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A5   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A6   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A7   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A8   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A9   | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A10  | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A11  | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A12  | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A13  | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A14  | Analog input, sent over CAN                | INPUT_PULLUP enabled         |
| A15  | Analog input, sent over CAN                | INPUT_PULLUP enabled         |

### Other / Special

| Pin  | Function / Assignment          | Notes                                      |
|------|--------------------------------|--------------------------------------------|
| AREF | Analog reference input         | Unused by firmware                         |
| SDA  | Alias of D20 (I2C SDA / VSS)   | I2C disabled; used as VSS INT1 (RearLeft)  |
| SCL  | Alias of D21 (I2C SCL / VSS)   | I2C disabled; used as VSS INT0 (RearRight) |

### Summary

- **Reserved:** D9 (CAN CS), D50–D53 (SPI)  
- **VSS inputs:** D18–D21 (external interrupts, pullups enabled, falling edge)  
- **Digital inputs:** D22–D37 (16-bit packed, INPUT_PULLUP, inverted logic)  
- **Analog inputs:** A0–A15  
- **PWM outputs (planned):** D2, D3, D5, D6, D7, D8, D11, D12, D44, D45, D46  
- **Low-speed outputs (planned):** D39, D40, D41, D42, D43, D47, D48, D49  
- **Spare GPIO/PWM:** D0, D1, D10, D14–D17, D38 (subject to peripheral use)  