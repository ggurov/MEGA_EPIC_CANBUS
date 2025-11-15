/*
  MEGA_EPIC_CANBUS - Arduino Mega2560 CAN I/O Expander Firmware
  --------------------------------------------------------------
  Expands epicEFI ECU input/output via CAN bus using the EPIC_CAN_BUS protocol.
  Provides analog inputs, PWM outputs, digital inputs/outputs for advanced ECU interfacing.
  
  Author: Gennady Gurov
  Made for: epicEFI project
*/


#include <SPI.h>
#include <mcp_canbus.h>
#include <stdint.h>

// Please modify SPI_CS_PIN to adapt to your board (see table below)
#define SPI_CS_PIN  9 

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

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

// VSS (Vehicle Speed Sensor) configuration
#define VSS_FRONT_LEFT_PIN   20  // INT1
#define VSS_FRONT_RIGHT_PIN  21  // INT0
#define VSS_REAR_LEFT_PIN    18  // INT3
#define VSS_REAR_RIGHT_PIN   19  // INT2
#define VSS_CALC_INTERVAL_MS 200 // Rate calculation interval
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

// Static buffer for CAN frame data (reused to avoid stack overflow)
static unsigned char canFrameData[8];

static inline void sendVariableSetFrame(int32_t varHash, float value)
{
    writeInt32BigEndian(varHash, &canFrameData[0]);
    writeFloat32BigEndian(value, &canFrameData[4]);
    CAN.sendMsgBuf(CAN_ID_VARIABLE_SET, 0, 8, canFrameData);
}

// ---------------- VSS Interrupt Service Routines ----------------
// ISR for FrontLeft (D20, INT1)
ISR(INT1_vect) {
    // Simple increment - prevent overflow by checking before increment
    if (vssChannels[0].edgeCount < 0xFFFFFFFE) {
        vssChannels[0].edgeCount++;
    }
}

// ISR for FrontRight (D21, INT0)
ISR(INT0_vect) {
    if (vssChannels[1].edgeCount < 0xFFFFFFFE) {
        vssChannels[1].edgeCount++;
    }
}

// ISR for RearLeft (D18, INT3)
ISR(INT3_vect) {
    if (vssChannels[2].edgeCount < 0xFFFFFFFE) {
        vssChannels[2].edgeCount++;
    }
}

// ISR for RearRight (D19, INT2)
ISR(INT2_vect) {
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
    
    
    while (CAN_OK != CAN.begin(CAN_500KBPS))    // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS FAIL!");
        delay(100);
    }
    Serial.println("CAN BUS OK!");
    delay(10);
    
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
    
    // Disable interrupts while configuring registers
    cli();
    
    // Configure external interrupts for falling edge detection
    // INT1 (D20): Falling edge (configured in EICRA)
    EICRA |= (1<<ISC11) | (0<<ISC10);
    
    // INT0 (D21): Falling edge (configured in EICRA)
    EICRA |= (1<<ISC01) | (0<<ISC00);
    
    // INT3 (D18): Falling edge (configured in EICRB)
    EICRB |= (1<<ISC31) | (0<<ISC30);
    
    // INT2 (D19): Falling edge (configured in EICRB)
    EICRB |= (1<<ISC21) | (0<<ISC20);
    
    // Enable external interrupts
    EIMSK |= (1<<INT1) | (1<<INT0) | (1<<INT3) | (1<<INT2);
    
    // Re-enable global interrupts
    sei();
    
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
    unsigned char len = 0;
    unsigned char buf[8];

    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf

        unsigned long canId = CAN.getCanId();
        
    }

    // Calculate VSS rates (runs every VSS_CALC_INTERVAL_MS)
    calculateVSSRates();
    
    unsigned long nowMs = millis();
    
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
        delayMicroseconds(200);
    }
    
    transmitIfNeeded(&digitalTxState, VAR_HASH_D22_D37, 
                    (float)currentDigitalBits, nowMs);
    delayMicroseconds(200);
    
    const int32_t vssHashes[4] = {
        VAR_HASH_VSS_FRONT_LEFT,
        VAR_HASH_VSS_FRONT_RIGHT,
        VAR_HASH_VSS_REAR_LEFT,
        VAR_HASH_VSS_REAR_RIGHT
    };
    
    for (uint8_t i = 0; i < 4; ++i) {
        transmitIfNeeded(&vssTxState[i], vssHashes[i], 
                        currentVssValues[i], nowMs);
        delayMicroseconds(200);
    }
}

// END FILE
