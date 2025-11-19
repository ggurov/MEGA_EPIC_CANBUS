
/*
  MEGA_EPIC_CANBUS - Arduino Mega2560 CAN I/O Expander Firmware
  --------------------------------------------------------------
  Expands epicEFI ECU input/output via CAN bus using the EPIC_CAN_BUS protocol.
  Provides analog inputs, PWM outputs, digital inputs/outputs for advanced ECU interfacing.
  
  Author: Gennady Gurov
  Made for: epicEFI project
*/


#include <SPI.h>
#include <stdint.h>
#include <PWFusion_MCP2515.h>
#include <PWFusion_MCP2515_Registers.h>

// Please modify SPI_CS_PIN to adapt to your board (see table below)
#define SPI_CS_PIN  9 

MCP2515 CAN;
// MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

// MCP2515 interrupt pin (D2, INT4 on Mega2560)
#define MCP2515_INT_PIN  2

// this is periodic request from us, ECU will broadcast on what we're listening for on state change
#define SLOW_OUT_REQUEST_INTERVAL_MS 50

// Note: CAN interrupt handling is managed by Playing With Fusion library internally
// No need for manual interrupt flag - library queues messages and CAN.receive() checks the queue

// ECU and EPIC CAN configuration
#define ECU_CAN_ID  1
#define CAN_ID_VAR_REQUEST        (0x700 + ECU_CAN_ID)
#define CAN_ID_VAR_RESPONSE       (0x720 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_REQUEST   (0x740 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_RESPONSE  (0x760 + ECU_CAN_ID)
#define CAN_ID_VARIABLE_SET       (0x780 + ECU_CAN_ID)

// Smart transmission configuration
#define TX_INTERVAL_FAST_MS    25   // Fast transmission for changed values
#define TX_INTERVAL_SLOW_MS    500  // Slow transmission for stable values (heartbeat)
#define TX_READ_INTERVAL_MS    25   // Input reading interval (independent of transmission)

// Change detection thresholds
#define TX_ANALOG_THRESHOLD    2.0f    // ADC counts (avoid noise)
#define TX_VSS_THRESHOLD       0.1f    // Pulses per second
// Digital: exact match (no threshold needed)

// EPIC variable hashes (from variables.json) for analog inputs A0-A15
// Order: A0..A15
const int32_t VAR_HASH_ANALOG[16] = {
    595545759,   // A0
    595545760,   // A1
    595545761,   // A2
    595545762,   // A3
    595545763,   // A4
    595545764,   // A5
    595545765,   // A6
    595545766,   // A7
    595545767,   // A8
    595545768,   // A9
   -1821826352,  // A10
   -1821826351,  // A11
   -1821826350,  // A12
   -1821826349,  // A13
   -1821826348,  // A14
   -1821826347   // A15
};

// EPIC variable hash for packed digital inputs D22-D37 bitfield (16 bits)
const int32_t VAR_HASH_D22_D37 = 2138825443;

// EPIC variable hash for slow GPIO and PWM outputs bitfield
// Bits 0-7: Slow GPIO (D39–D43, D47–D49)
// Bits 8-17: PWM outputs (D3, D5, D6, D7, D8, D11, D12, D44, D45, D46)
const int32_t VAR_HASH_OUT_SLOW = 1430780106;

// VSS (Vehicle Speed Sensor) configuration
// Wheel-to-pin mapping:
// - FrontLeft  -> D18 (INT3)
// - FrontRight -> D19 (INT2)
// - RearLeft   -> D20 (INT1)
// - RearRight  -> D21 (INT0)
#define VSS_FRONT_LEFT_PIN   18  // INT3
#define VSS_FRONT_RIGHT_PIN  19  // INT2
#define VSS_REAR_LEFT_PIN    20  // INT1
#define VSS_REAR_RIGHT_PIN   21  // INT0
#define VSS_CALC_INTERVAL_MS 25 // Rate calculation interval
#define VSS_ENABLE_PULLUP    1   // Internal pullups enabled to prevent interrupt storms when pins are floating

// VSS variable hashes (from vss_pickup.txt)
const int32_t VAR_HASH_VSS_FRONT_LEFT  = -1645222329;
const int32_t VAR_HASH_VSS_FRONT_RIGHT = 1549498074;
const int32_t VAR_HASH_VSS_REAR_LEFT   = 768443592;
const int32_t VAR_HASH_VSS_REAR_RIGHT  = -403905157;

// VSS counter structure
struct VSSChannel {
    volatile uint32_t edgeCount;      // Edge counter (ISR increments)
    uint32_t lastCount;               // Last count for rate calculation
    unsigned long lastCalcTime;       // Last calculation timestamp (ms)
    float pulsesPerSecond;             // Calculated rate
};

// VSS channels array (FrontLeft, FrontRight, RearLeft, RearRight)
VSSChannel vssChannels[4] = {
    {0, 0, 0, 0.0f},  // FrontLeft
    {0, 0, 0, 0.0f},  // FrontRight
    {0, 0, 0, 0.0f},  // RearLeft
    {0, 0, 0, 0.0f}   // RearRight
};

// Smart transmission state machine
#define TX_STATE_CHANGED  0  // Value changed, transmit quickly
#define TX_STATE_STABLE   1  // Value stable, transmit slowly

struct TxChannelState {
    float lastTransmittedValue;    // Last value sent over CAN
    unsigned long lastTxTime;       // Timestamp of last transmission (ms)
    bool hasChanged;                // True if value changed since last TX
    uint8_t state;                  // State machine state (CHANGED/STABLE)
};

// Transmission state arrays
TxChannelState analogTxState[16];
TxChannelState digitalTxState;
TxChannelState vssTxState[4];

// Current input values (updated during read cycle, used for transmission)
static float currentAnalogValues[16] = {0};
static uint16_t currentDigitalBits = 0;
static float currentVssValues[4] = {0};

// Slow GPIO output pins (bit0..bit7 → D39, D40, D41, D42, D43, D47, D48, D49)
const uint8_t SLOW_GPIO_PINS[8] = {39, 40, 41, 42, 43, 47, 48, 49};

// PWM output pins (bit8..bit17 → D3, D5, D6, D7, D8, D11, D12, D44, D45, D46)
// Order matches ECU bit packing: bit8=D3, bit9=D5, bit10=D6, bit11=D7, bit12=D8,
// bit13=D11, bit14=D12, bit15=D44, bit16=D45, bit17=D46
const uint8_t PWM_OUTPUT_PINS[10] = {3, 5, 6, 7, 8, 11, 12, 44, 45, 46};

// ---------------- Internal helpers ----------------
static inline void writeInt32BigEndian(int32_t value, unsigned char* out)
{
    out[0] = (unsigned char)((value >> 24) & 0xFF);
    out[1] = (unsigned char)((value >> 16) & 0xFF);
    out[2] = (unsigned char)((value >> 8) & 0xFF);
    out[3] = (unsigned char)(value & 0xFF);
}

static inline void writeFloat32BigEndian(float value, unsigned char* out)
{
    union { float f; uint32_t u; } conv;
    conv.f = value;
    out[0] = (unsigned char)((conv.u >> 24) & 0xFF);
    out[1] = (unsigned char)((conv.u >> 16) & 0xFF);
    out[2] = (unsigned char)((conv.u >> 8) & 0xFF);
    out[3] = (unsigned char)(conv.u & 0xFF);
}

static inline int32_t readInt32BigEndian(const unsigned char* in)
{
    int32_t value = 0;
    value |= ((int32_t)in[0] << 24);
    value |= ((int32_t)in[1] << 16);
    value |= ((int32_t)in[2] << 8);
    value |= ((int32_t)in[3]);
    return value;
}

static inline float readFloat32BigEndian(const unsigned char* in)
{
    union { float f; uint32_t u; } conv;
    conv.u =  ((uint32_t)in[0] << 24)
            | ((uint32_t)in[1] << 16)
            | ((uint32_t)in[2] << 8)
            | ((uint32_t)in[3]);
    return conv.f;
}

// Static CAN message structure for sending (reused to avoid stack/heap churn)
static can_message_t txMsg;

static inline void sendVariableSetFrame(int32_t varHash, float value)
{
    // Prepare message structure
    txMsg.sid = CAN_ID_VARIABLE_SET;
    txMsg.extended = 0;  // Standard 11-bit ID
    txMsg.rtr = 0;       // Data frame
    txMsg.dlc = 8;       // 8 bytes: 4-byte hash + 4-byte float
    
    // Write hash and value to data array (big-endian)
    writeInt32BigEndian(varHash, &txMsg.data[0]);
    writeFloat32BigEndian(value, &txMsg.data[4]);
    
    // Send message
    CAN.send(&txMsg);
}

static inline void sendVariableRequestFrame(int32_t varHash)
{
    // Prepare message structure
    txMsg.sid = CAN_ID_VAR_REQUEST;
    txMsg.extended = 0;  // Standard 11-bit ID
    txMsg.rtr = 0;       // Data frame
    txMsg.dlc = 4;       // 4 bytes: hash only
    
    // Write hash to data array (big-endian)
    writeInt32BigEndian(varHash, &txMsg.data[0]);
    
    // Send message
    CAN.send(&txMsg);
}

// ---------------- CAN Filter Configuration ----------------
// Configure MCP2515 hardware filters to accept only CAN_ID_VAR_RESPONSE
// This reduces CPU overhead by filtering unwanted messages at hardware level
static void configureCANFilters()
{
    // MCP2515 filter configuration requires direct register access
    // We need to put MCP2515 in configuration mode, set filters, then return to normal mode
    
    // Note: This function uses SPI directly to access MCP2515 registers
    // The library may have its own methods, but direct access ensures it works
    
    // MCP2515 register addresses (from PWFusion_MCP2515_Registers.h)
    // RXF0: 0x00, RXF1: 0x04, RXF2: 0x08, RXF3: 0x10, RXF4: 0x14, RXF5: 0x18
    // RXM0: 0x20, RXM1: 0x24
    // CANCTRL: 0x0F (mode control)
    
    // Standard 11-bit ID format in MCP2515 registers:
    // Bits 10-3 in SIDH (high byte), bits 2-0 in SIDL (low byte, upper 3 bits)
    // For CAN_ID_VAR_RESPONSE (0x721):
    //   0x721 = 0000 0111 0010 0001
    //   SIDH = 0x72 (bits 10-3: 0000 0111 0010)
    //   SIDL = 0x20 (bits 2-0: 000, extended ID bit = 0)
    
    uint16_t filterId = CAN_ID_VAR_RESPONSE;
    uint8_t sidh = (uint8_t)((filterId >> 3) & 0xFF);
    uint8_t sidl = (uint8_t)((filterId & 0x07) << 5);  // Standard ID, no extended
    
    // Mask: 0x7FF (all 11 bits must match)
    uint8_t maskSidh = 0xFF;
    uint8_t maskSidl = 0xE0;  // Bits 7-5 = 111 (match all 3 bits)
    
    // Start SPI transaction
    SPI.beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    
    // Read CANCTRL to get current mode
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x03);  // READ instruction
    SPI.transfer(0x0F);  // CANCTRL register
    uint8_t canctrl = SPI.transfer(0x00);
    digitalWrite(SPI_CS_PIN, HIGH);
    
    // Put MCP2515 in configuration mode (REQOP = 100)
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x0F);  // CANCTRL register
    SPI.transfer((canctrl & 0x8F) | 0x80);  // Set REQOP bits to 100 (config mode)
    digitalWrite(SPI_CS_PIN, HIGH);
    delayMicroseconds(10);  // Wait for mode change
    
    // Configure RXB0 mask (RXM0) - match all 11 bits
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x20);  // RXM0SIDH
    SPI.transfer(maskSidh);
    SPI.transfer(maskSidl);
    SPI.transfer(0x00);  // EID8
    SPI.transfer(0x00);  // EID0
    digitalWrite(SPI_CS_PIN, HIGH);
    
    // Configure RXB0 filter 0 (RXF0) - match CAN_ID_VAR_RESPONSE
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x00);  // RXF0SIDH
    SPI.transfer(sidh);
    SPI.transfer(sidl);
    SPI.transfer(0x00);  // EID8
    SPI.transfer(0x00);  // EID0
    digitalWrite(SPI_CS_PIN, HIGH);
    
    // Configure RXB0 filter 1 (RXF1) - match CAN_ID_VAR_RESPONSE
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x04);  // RXF1SIDH
    SPI.transfer(sidh);
    SPI.transfer(sidl);
    SPI.transfer(0x00);  // EID8
    SPI.transfer(0x00);  // EID0
    digitalWrite(SPI_CS_PIN, HIGH);
    
    // Configure RXB1 mask (RXM1) - match all 11 bits
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x24);  // RXM1SIDH
    SPI.transfer(maskSidh);
    SPI.transfer(maskSidl);
    SPI.transfer(0x00);  // EID8
    SPI.transfer(0x00);  // EID0
    digitalWrite(SPI_CS_PIN, HIGH);
    
    // Configure RXB1 filter 2 (RXF2) - match CAN_ID_VAR_RESPONSE
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x08);  // RXF2SIDH
    SPI.transfer(sidh);
    SPI.transfer(sidl);
    SPI.transfer(0x00);  // EID8
    SPI.transfer(0x00);  // EID0
    digitalWrite(SPI_CS_PIN, HIGH);
    
    // Return to normal mode (REQOP = 000)
    digitalWrite(SPI_CS_PIN, LOW);
    SPI.transfer(0x02);  // WRITE instruction
    SPI.transfer(0x0F);  // CANCTRL register
    SPI.transfer(canctrl & 0x8F);  // Clear REQOP bits (normal mode)
    digitalWrite(SPI_CS_PIN, HIGH);
    
    SPI.endTransaction();
    delayMicroseconds(10);  // Wait for mode change
}

// ---------------- CAN Interrupt Service Routine ----------------
// Note: Playing With Fusion library handles CAN interrupt setup internally via attachInterrupt()
// We don't define ISR(INT4_vect) here to avoid conflict with the library's interrupt handling.
// The library's CAN.receive() method will work with its internal interrupt mechanism.

// ---------------- VSS Interrupt Service Routines ----------------
// Use attachInterrupt() instead of direct ISR definitions to avoid conflicts with library
// FrontLeft (D18, INT3)
void vssFrontLeftISR() {
    if (vssChannels[0].edgeCount < 0xFFFFFFFE) {
        vssChannels[0].edgeCount++;
    }
}

// FrontRight (D19, INT2)
void vssFrontRightISR() {
    if (vssChannels[1].edgeCount < 0xFFFFFFFE) {
        vssChannels[1].edgeCount++;
    }
}

// RearLeft (D20, INT1)
void vssRearLeftISR() {
    if (vssChannels[2].edgeCount < 0xFFFFFFFE) {
        vssChannels[2].edgeCount++;
    }
}

// RearRight (D21, INT0)
void vssRearRightISR() {
    if (vssChannels[3].edgeCount < 0xFFFFFFFE) {
        vssChannels[3].edgeCount++;
    }
}

// ---------------- Smart Transmission Functions ----------------
void readAnalogInputs(float* values) {
    for (uint8_t i = 0; i < 16; ++i) {
        values[i] = (float)analogRead(A0 + i);
    }
}

void readDigitalInputs(uint16_t* bits) {
    *bits = 0;
    for (uint8_t pin = 22; pin <= 37; ++pin) {
        uint8_t bitIndex = (uint8_t)(pin - 22);
        int state = digitalRead(pin);
        if (state == LOW) {  // Grounded = report as 1
            *bits |= (uint16_t)(1u << bitIndex);
        }
    }
}

void readVSSInputs(float* values) {
    // VSS values already calculated in calculateVSSRates()
    for (uint8_t i = 0; i < 4; ++i) {
        values[i] = vssChannels[i].pulsesPerSecond;
    }
}

bool hasAnalogChanged(uint8_t channel, float newValue) {
    float diff = newValue - analogTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff; // abs
    return (diff >= TX_ANALOG_THRESHOLD);
}

bool hasDigitalChanged(uint16_t newBits) {
    return (newBits != (uint16_t)digitalTxState.lastTransmittedValue);
}

bool hasVSSChanged(uint8_t channel, float newValue) {
    float diff = newValue - vssTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff; // abs
    return (diff >= TX_VSS_THRESHOLD);
}

void updateTxState(TxChannelState* state, bool changed, unsigned long nowMs) {
    if (changed) {
        state->hasChanged = true;
        state->state = TX_STATE_CHANGED;
    } else {
        state->hasChanged = false;
        // Transition to STABLE only after fast transmission
        if (state->state == TX_STATE_CHANGED && 
            (nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS) {
            state->state = TX_STATE_STABLE;
        }
    }
}

bool shouldTransmit(TxChannelState* state, unsigned long nowMs) {
    if (state->hasChanged) {
        // Changed: transmit if fast interval elapsed
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS);
    } else {
        // Stable: transmit if slow interval elapsed
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_SLOW_MS);
    }
}

void transmitIfNeeded(TxChannelState* state, int32_t varHash, float value, unsigned long nowMs) {
    if (shouldTransmit(state, nowMs)) {
        sendVariableSetFrame(varHash, value);
        state->lastTransmittedValue = value;
        state->lastTxTime = nowMs;
        state->hasChanged = false;
        if (state->state == TX_STATE_CHANGED) {
            state->state = TX_STATE_STABLE;
        }
//        delayMicroseconds(200);
    }
}

// ---------------- VSS Rate Calculation ----------------
void calculateVSSRates() {
    static unsigned long lastCalcTime = 0;
    unsigned long now = millis();
    
    // Handle millis() overflow (wraps every ~49.7 days)
    // Unsigned subtraction handles wrap correctly if interval is reasonable
    unsigned long timeDelta = now - lastCalcTime;
    
    // Check for valid time delta (handles overflow case where now < lastCalcTime)
    // Also ensure delta is reasonable (not more than 1 second, which would indicate overflow issue)
    if (timeDelta >= VSS_CALC_INTERVAL_MS && timeDelta < 1000) {
        float timeDeltaSeconds = timeDelta / 1000.0f;
        
        for (uint8_t i = 0; i < 4; ++i) {
            // Atomic read of volatile counter
            uint32_t currentCount = vssChannels[i].edgeCount;
            // Unsigned subtraction handles counter overflow correctly
            uint32_t countDelta = currentCount - vssChannels[i].lastCount;
            
            // Calculate pulses per second
            vssChannels[i].pulsesPerSecond = countDelta / timeDeltaSeconds;
            vssChannels[i].lastCount = currentCount;
        }
        lastCalcTime = now;
    }
    // If timeDelta is too large (overflow case), skip calculation and reset
    else if (timeDelta >= 1000) {
        // Likely overflow occurred, reset timing
        lastCalcTime = now;
        // Optionally reset counters or set rates to 0
        for (uint8_t i = 0; i < 4; ++i) {
            vssChannels[i].lastCount = vssChannels[i].edgeCount;
            vssChannels[i].pulsesPerSecond = 0.0f;
        }
    }
}



void setup()
{
    Serial.begin(115200);
    while(!Serial);
    
    
    // Initialize CAN bus: CS pin, INT pin, loopback=false, 500kbps
    // Playing With Fusion library begin() returns true on success
    CAN.begin(SPI_CS_PIN, MCP2515_INT_PIN, false, 500);
    // {
    //     Serial.println("CAN BUS FAIL!");
    //     delay(100);
    // }
    Serial.println("CAN BUS OK!");
    delay(10);
    
    // Configure MCP2515 hardware filters to accept only CAN_ID_VAR_RESPONSE (0x721)
    // This reduces CPU overhead by filtering unwanted messages at hardware level
    configureCANFilters();
    
    // Note: Playing With Fusion library's begin() method handles CAN interrupt setup
    // internally via attachInterrupt(). No manual interrupt configuration needed for CAN.
    
    // Configure analog inputs A0-A15 with internal pullup enabled
    // Note: Pullup will pull floating pins to ~5V (affects ADC readings)
    for (uint8_t i = 0; i < 16; ++i)
    {
        pinMode(A0 + i, INPUT_PULLUP);
    }
    
    // Configure digital button inputs D22-D37 (16 bits)
    for (uint8_t pin = 22; pin <= 37; ++pin)
    {
        pinMode(pin, INPUT_PULLUP);
    }
    
    // Configure slow GPIO outputs (low-speed outputs)
    for (uint8_t i = 0; i < 8; ++i)
    {
        pinMode(SLOW_GPIO_PINS[i], OUTPUT);
        digitalWrite(SLOW_GPIO_PINS[i], LOW);
    }
    
    // Configure PWM output pins
    for (uint8_t i = 0; i < 10; ++i)
    {
        pinMode(PWM_OUTPUT_PINS[i], OUTPUT);
        analogWrite(PWM_OUTPUT_PINS[i], 0);  // Start with 0% duty cycle
    }

    // Disable I2C (TWEN bit in TWCR) to free D20/D21 for VSS interrupts
    // This prevents conflicts with I2C hardware
    TWCR &= ~(1<<TWEN);
    
    // Configure VSS interrupt pins (D18, D19, D20, D21)
    // Pullups enabled to prevent interrupt storms when pins are floating
    // VR conditioner signals will override pullup when connected
    #if VSS_ENABLE_PULLUP
        pinMode(VSS_FRONT_LEFT_PIN, INPUT_PULLUP);
        pinMode(VSS_FRONT_RIGHT_PIN, INPUT_PULLUP);
        pinMode(VSS_REAR_LEFT_PIN, INPUT_PULLUP);
        pinMode(VSS_REAR_RIGHT_PIN, INPUT_PULLUP);
    #else
        pinMode(VSS_FRONT_LEFT_PIN, INPUT);
        pinMode(VSS_FRONT_RIGHT_PIN, INPUT);
        pinMode(VSS_REAR_LEFT_PIN, INPUT);
        pinMode(VSS_REAR_RIGHT_PIN, INPUT);
    #endif
    
    // Configure VSS interrupts using attachInterrupt() to avoid conflicts with library
    // This uses Arduino's interrupt system which is compatible with the Playing With Fusion library
    attachInterrupt(digitalPinToInterrupt(VSS_FRONT_LEFT_PIN), vssFrontLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_FRONT_RIGHT_PIN), vssFrontRightISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_REAR_LEFT_PIN), vssRearLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_REAR_RIGHT_PIN), vssRearRightISR, FALLING);
    
    // Initialize smart transmission state structures
    for (uint8_t i = 0; i < 16; ++i) {
        analogTxState[i].lastTransmittedValue = 0.0f;
        analogTxState[i].lastTxTime = 0;
        analogTxState[i].hasChanged = true;  // Force initial transmission
        analogTxState[i].state = TX_STATE_CHANGED;
    }
    
    digitalTxState.lastTransmittedValue = 0.0f;
    digitalTxState.lastTxTime = 0;
    digitalTxState.hasChanged = true;  // Force initial transmission
    digitalTxState.state = TX_STATE_CHANGED;
    
    for (uint8_t i = 0; i < 4; ++i) {
        vssTxState[i].lastTransmittedValue = 0.0f;
        vssTxState[i].lastTxTime = 0;
        vssTxState[i].hasChanged = true;  // Force initial transmission
        vssTxState[i].state = TX_STATE_CHANGED;
    }
}


void loop()
{
    can_message_t rxMsg; // Allocate memory to store received CAN messages

    // Process CAN frames using library's receive method
    // Playing With Fusion library handles interrupts internally and queues messages
    // CAN.receive() returns true if a message is available, false when queue is empty
    uint8_t frameCount = 0;
    while (CAN.receive(&rxMsg))
    {
        frameCount++;

        // Handle variable response frames for slow GPIO and PWM outputs
        if (rxMsg.sid == CAN_ID_VAR_RESPONSE && rxMsg.dlc == 8)
        {
            int32_t hash = readInt32BigEndian(&rxMsg.data[0]);
            if (hash == VAR_HASH_OUT_SLOW)
            {
                float value = readFloat32BigEndian(&rxMsg.data[4]);

                // Treat value as uint32_t bitfield (18 bits: 0-7 slow GPIO, 8-17 PWM)
                // ECU packs: bits 0-7 = slow GPIO, bits 8-17 = PWM outputs
                uint32_t rawBits = (value >= 0.0f) ? (uint32_t)(value + 0.5f) : 0u;
                
                // Extract slow GPIO bits (0-7) and set digital outputs
                uint8_t slowBits = (uint8_t)(rawBits & 0xFFu);
                for (uint8_t i = 0; i < 8; ++i)
                {
                    uint8_t pin = SLOW_GPIO_PINS[i];
                    if (slowBits & (1u << i)) {
                        digitalWrite(pin, HIGH);
                    } else {
                        digitalWrite(pin, LOW);
                    }
                }
                
                // Extract PWM bits (8-17) and set PWM duty cycles
                // Each PWM bit represents on/off state (0 = 0%, 1 = 100% duty cycle)
                // Note: Future enhancement could use full 8-bit PWM values if ECU provides them
                for (uint8_t i = 0; i < 10; ++i)
                {
                    uint8_t pin = PWM_OUTPUT_PINS[i];
                    uint8_t bitIndex = i + 8;  // Bits 8-17
                    if (rawBits & (1u << bitIndex)) {
                        analogWrite(pin, 255);  // 100% duty cycle (ON)
                    } else {
                        analogWrite(pin, 0);    // 0% duty cycle (OFF)
                    }
                }
            }
        }
    }
    

    // Calculate VSS rates (runs every VSS_CALC_INTERVAL_MS)
    calculateVSSRates();
    
    unsigned long nowMs = millis();

    // Periodically request slow GPIO output state from ECU
    static unsigned long lastSlowOutRequestMs = 0;

    if (nowMs - lastSlowOutRequestMs >= SLOW_OUT_REQUEST_INTERVAL_MS)
    {
        lastSlowOutRequestMs = nowMs;
        sendVariableRequestFrame(VAR_HASH_OUT_SLOW);
    }
    
    // Read inputs at fixed interval (independent of transmission)
    static unsigned long lastReadMs = 0;
    if (nowMs - lastReadMs >= TX_READ_INTERVAL_MS) {
        lastReadMs = nowMs;
        
        // Read all inputs
        readAnalogInputs(currentAnalogValues);
        readDigitalInputs(&currentDigitalBits);
        readVSSInputs(currentVssValues);
        
        // Update change detection and state machines
        for (uint8_t i = 0; i < 16; ++i) {
            bool changed = hasAnalogChanged(i, currentAnalogValues[i]);
            updateTxState(&analogTxState[i], changed, nowMs);
        }
        
        bool digitalChanged = hasDigitalChanged(currentDigitalBits);
        updateTxState(&digitalTxState, digitalChanged, nowMs);
        
        for (uint8_t i = 0; i < 4; ++i) {
            bool changed = hasVSSChanged(i, currentVssValues[i]);
            updateTxState(&vssTxState[i], changed, nowMs);
        }
    }
    
    // Transmit based on smart scheduling (runs every loop iteration)
    for (uint8_t i = 0; i < 16; ++i) {
        transmitIfNeeded(&analogTxState[i], VAR_HASH_ANALOG[i], 
                        currentAnalogValues[i], nowMs);
        
    }
    
    transmitIfNeeded(&digitalTxState, VAR_HASH_D22_D37, 
                    (float)currentDigitalBits, nowMs);
    
    const int32_t vssHashes[4] = {
        VAR_HASH_VSS_FRONT_LEFT,
        VAR_HASH_VSS_FRONT_RIGHT,
        VAR_HASH_VSS_REAR_LEFT,
        VAR_HASH_VSS_REAR_RIGHT
    };
    
    for (uint8_t i = 0; i < 4; ++i) {
        transmitIfNeeded(&vssTxState[i], vssHashes[i], 
                        currentVssValues[i], nowMs);
    }
}

// END FILE
