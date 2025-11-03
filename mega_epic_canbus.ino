/*
    MEGA_EPIC_CANBUS - Arduino Mega2560 CAN Bus I/O Expansion Firmware
    
    Version: 1.0.0
    Status: Production Ready
    License: See LICENSE file
    
    Purpose:
    Expands I/O capabilities of epicEFI ECUs via CAN bus communication.
    Implements EPIC_CAN_BUS protocol for remote variable access and function calls.
    
    Hardware:
    - Arduino Mega2560 (or compatible)
    - MCP_CAN Shield (MCP2515-based CAN controller)
    - SPI CS Pin: D9 (default, configurable)
    
    I/O Capabilities:
    - 16 Analog Inputs (A0-A15): 10-bit ADC, 0-5V
    - 15 Digital Inputs (D20-D34): Pullup-enabled, LOW=active
    - 15 Digital Outputs (D35-D49): ECU-controlled
    - 14 PWM Outputs (D2-D8, D10-D13, D44-D46): 8-bit PWM
    - 2 Interrupt Counters (D18, D19): Optional, for wheel speed sensors
    
    Documentation:
    - README.md: User documentation and quick start
    - CONFIGURATION.md: Configuration guide
    - PIN_ASSIGNMENT.md: Pin map and wiring diagrams
    - TROUBLESHOOTING.md: Troubleshooting guide
    - TECHNICAL.md: Technical documentation
    - INSTALLATION.md: Installation instructions
    
    Protocol:
    - EPIC Over CANbus Protocol (see .project/epic_can_bus_spec.txt)
    - CAN ID Base Offsets: 0x700 (var_request), 0x720 (var_response),
                           0x740 (func_request), 0x760 (func_response),
                           0x780 (var_set)
    - Data Format: Big-endian byte order, Float32 IEEE 754
    
    CAN Baudrate,
    
    #define CAN_5KBPS           1
    #define CAN_10KBPS          2
    #define CAN_20KBPS          3
    #define CAN_25KBPS          4 
    #define CAN_31K25BPS        5
    #define CAN_33KBPS          6
    #define CAN_40KBPS          7
    #define CAN_50KBPS          8
    #define CAN_80KBPS          9
    #define CAN_83K3BPS         10
    #define CAN_95KBPS          11
    #define CAN_100KBPS         12
    #define CAN_125KBPS         13
    #define CAN_200KBPS         14
    #define CAN_250KBPS         15
    #define CAN_500KBPS         16
    #define CAN_666KBPS         17
    #define CAN_1000KBPS        18

    CANBed V1: https://www.longan-labs.cc/1030008.html
    CANBed M0: https://www.longan-labs.cc/1030014.html
    CAN Bus Shield: https://www.longan-labs.cc/1030016.html
    OBD-II CAN Bus GPS Dev Kit: https://www.longan-labs.cc/1030003.html
*/

#include <SPI.h>
#include <mcp_canbus.h>
#include <stdint.h>
#include <EEPROM.h>

// Please modify SPI_CS_PIN to adapt to your board (see table below)
#define SPI_CS_PIN  9 

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

// ECU and EPIC CAN configuration
#define ECU_CAN_ID  1
#define TRANSMIT_INTERVAL_MS 25
#define DIGITAL_OUTPUT_POLL_INTERVAL_MS 50  // 20 Hz polling for digital outputs
#define PWM_POLL_INTERVAL_MS 100  // 10 Hz polling for PWM outputs (14 channels, staggered)

// Performance optimization: Change detection thresholds
#define ANALOG_CHANGE_THRESHOLD 5.0  // ADC counts - only transmit if change >= this value
#define ANALOG_HEARTBEAT_INTERVAL_MS 1000  // Send all analog values every 1 second even if unchanged
#define DIGITAL_CHANGE_ONLY true  // Only transmit digital inputs when state changes (no heartbeat needed for buttons)

// Error handling and robustness configuration
#define ECU_COMM_TIMEOUT_MS 3000  // ECU communication timeout (3 seconds)
#define CAN_TX_RETRY_COUNT 2  // Number of retries for CAN TX failures
#define CAN_TX_RETRY_DELAY_MS 10  // Delay between retries (ms)

// Advanced features configuration flags
#define ENABLE_INTERRUPT_COUNTERS false  // Set to true to enable interrupt counters (D18, D19)
#define ENABLE_ADC_CALIBRATION false      // Set to true to enable ADC calibration
#define ENABLE_EEPROM_CONFIG true         // Enable EEPROM configuration storage

// Testing and diagnostics configuration
#define ENABLE_TEST_MODE false            // Set to true to enable test mode (I/O patterns, diagnostics)
#define ENABLE_SERIAL_COMMANDS false      // Set to true to enable serial command interface
#define PERFORMANCE_STATS_INTERVAL_MS 10000  // Print performance stats every 10 seconds (0 = disabled)

// Interrupt counter pins (INT3=18, INT2=19) - also UART1 pins, use if UART not needed
#define COUNTER_PIN_1 18  // INT3
#define COUNTER_PIN_2 19  // INT2

// EEPROM addresses for configuration storage
#define EEPROM_ADDR_MAGIC       0  // Magic number to detect valid config (4 bytes)
#define EEPROM_ADDR_ECU_CAN_ID  4  // Stored ECU_CAN_ID (1 byte)
#define EEPROM_ADDR_CALIBRATION 8  // Start of ADC calibration data (32 bytes: 16 channels * 2 floats)

// EEPROM magic number to validate stored configuration
#define EEPROM_MAGIC_NUMBER 0x4D455049  // "MEPI" in ASCII
#define CAN_ID_VAR_REQUEST        (0x700 + ECU_CAN_ID)
#define CAN_ID_VAR_RESPONSE       (0x720 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_REQUEST   (0x740 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_RESPONSE  (0x760 + ECU_CAN_ID)
#define CAN_ID_VARIABLE_SET       (0x780 + ECU_CAN_ID)

// EPIC frame type identification helpers
#define IS_VAR_RESPONSE(id) ((id) == CAN_ID_VAR_RESPONSE)
#define IS_FUNCTION_RESPONSE(id) ((id) == CAN_ID_FUNCTION_RESPONSE)
#define IS_VAR_SET(id) ((id) == CAN_ID_VARIABLE_SET)

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

// EPIC variable hash for packed digital inputs D20-D34 bitfield
const int32_t VAR_HASH_D20_D34 = 2136453598;

// EPIC variable hash for packed digital outputs D35-D49 bitfield
// TODO: Get actual hash from variables.json - placeholder for now
const int32_t VAR_HASH_D35_D49 = 0;  // PLACEHOLDER - needs to be set from variables.json

// Performance optimization: Previous value tracking for change detection
static float previousAnalogValues[16] = {0};  // Track previous ADC values for A0-A15
static uint16_t previousDigitalBits = 0;      // Track previous digital input bitfield
static unsigned long lastAnalogHeartbeatMs = 0;  // Last heartbeat transmission time

// Error handling and diagnostics
static unsigned long lastEcuResponseMs = 0;  // Timestamp of last successful ECU response
static bool ecuCommunicationLost = false;   // ECU communication state
static uint32_t canTxSuccessCount = 0;      // CAN TX success counter
static uint32_t canTxFailureCount = 0;      // CAN TX failure counter
static uint32_t canRxCount = 0;             // CAN RX frame counter
static uint32_t canRxErrorCount = 0;        // CAN RX error counter (invalid frames)

// Performance statistics (for testing/validation)
static unsigned long statsStartTimeMs = 0;  // Statistics collection start time
static uint32_t statsStartTxSuccess = 0;    // TX success count at stats start
static uint32_t statsStartTxFailure = 0;    // TX failure count at stats start
static uint32_t statsStartRxCount = 0;      // RX count at stats start
static uint32_t statsStartRxError = 0;      // RX error count at stats start

// Test mode variables
#if ENABLE_TEST_MODE
static bool testModeActive = false;
static unsigned long testModeStartMs = 0;
#endif

// Interrupt counters (optional feature)
#if ENABLE_INTERRUPT_COUNTERS
volatile uint32_t interruptCounter1 = 0;  // Counter for D18 (INT3)
volatile uint32_t interruptCounter2 = 0;  // Counter for D19 (INT2)
volatile unsigned long lastCounter1TimeMs = 0;  // Timestamp of last counter1 event
volatile unsigned long lastCounter2TimeMs = 0;  // Timestamp of last counter2 event
static uint32_t lastReportedCounter1 = 0;  // Last reported counter value
static uint32_t lastReportedCounter2 = 0;  // Last reported counter value
#define COUNTER_REPORT_INTERVAL_MS 100  // Report counters every 100ms

// EPIC variable hashes for interrupt counters (TODO: get from variables.json)
const int32_t VAR_HASH_COUNTER1 = 0;  // PLACEHOLDER
const int32_t VAR_HASH_COUNTER2 = 0;  // PLACEHOLDER

// Interrupt Service Routines for counters
void counter1ISR(void)
{
    interruptCounter1++;
    lastCounter1TimeMs = millis();
}

void counter2ISR(void)
{
    interruptCounter2++;
    lastCounter2TimeMs = millis();
}
#endif

// ADC Calibration (optional feature)
#if ENABLE_ADC_CALIBRATION
// Calibration structure: offset and gain per channel
struct AdcCalibration {
    float offset;  // ADC offset (added to raw value)
    float gain;    // ADC gain multiplier
};

static AdcCalibration adcCal[16];  // Calibration for A0-A15

// Apply calibration to ADC value
static inline float applyAdcCalibration(uint8_t channel, float rawAdc)
{
    return (rawAdc + adcCal[channel].offset) * adcCal[channel].gain;
}

// Load ADC calibration from EEPROM
static void loadAdcCalibration(void)
{
    if (ENABLE_EEPROM_CONFIG)
    {
        uint16_t addr = EEPROM_ADDR_CALIBRATION;
        for (uint8_t i = 0; i < 16; ++i)
        {
            EEPROM.get(addr, adcCal[i].offset);
            addr += sizeof(float);
            EEPROM.get(addr, adcCal[i].gain);
            addr += sizeof(float);
        }
    }
}

// Save ADC calibration to EEPROM
static void saveAdcCalibration(void)
{
    if (ENABLE_EEPROM_CONFIG)
    {
        uint16_t addr = EEPROM_ADDR_CALIBRATION;
        for (uint8_t i = 0; i < 16; ++i)
        {
            EEPROM.put(addr, adcCal[i].offset);
            addr += sizeof(float);
            EEPROM.put(addr, adcCal[i].gain);
            addr += sizeof(float);
        }
    }
}
#endif

// EEPROM Configuration Functions
#if ENABLE_EEPROM_CONFIG
// Check if EEPROM contains valid configuration
static bool isEepromConfigValid(void)
{
    uint32_t magic;
    EEPROM.get(EEPROM_ADDR_MAGIC, magic);
    return (magic == EEPROM_MAGIC_NUMBER);
}

// Load ECU_CAN_ID from EEPROM (returns default if not configured)
static uint8_t loadEcuCanId(void)
{
    if (isEepromConfigValid())
    {
        uint8_t storedId;
        EEPROM.get(EEPROM_ADDR_ECU_CAN_ID, storedId);
        if (storedId <= 15)  // Validate range
        {
            return storedId;
        }
    }
    return ECU_CAN_ID;  // Return default
}

// Save ECU_CAN_ID to EEPROM
static void saveEcuCanId(uint8_t canId)
{
    if (canId > 15) return;  // Validate range
    
    // Write magic number
    EEPROM.put(EEPROM_ADDR_MAGIC, (uint32_t)EEPROM_MAGIC_NUMBER);
    // Write ECU_CAN_ID
    EEPROM.put(EEPROM_ADDR_ECU_CAN_ID, canId);
}
#endif

// PWM output pins (14 total: D2-D8, D10-D13, D44-D46, skipping D9 for MCP_CAN CS)
const uint8_t PWM_PINS[14] = {
    2, 3, 4, 5, 6, 7, 8,      // D2-D8
    10, 11, 12, 13,           // D10-D13
    44, 45, 46                // D44-D46
};

// EPIC variable hashes for PWM duty cycle values (0-100% float32)
// Order matches PWM_PINS array: D2, D3, D4, D5, D6, D7, D8, D10, D11, D12, D13, D44, D45, D46
// TODO: Get actual hashes from variables.json - all zeros are placeholders
const int32_t VAR_HASH_PWM[14] = {
    0, 0, 0, 0, 0, 0, 0,      // D2-D8 placeholders
    0, 0, 0, 0,               // D10-D13 placeholders
    0, 0, 0                   // D44-D46 placeholders
};

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
    return ((int32_t)in[0] << 24) | ((int32_t)in[1] << 16) | 
           ((int32_t)in[2] << 8) | (int32_t)in[3];
}

static inline float readFloat32BigEndian(const unsigned char* in)
{
    union { float f; uint32_t u; } conv;
    conv.u = ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | 
             ((uint32_t)in[2] << 8) | (uint32_t)in[3];
    return conv.f;
}

// Enhanced CAN TX with error handling and retry logic
static bool sendVariableSetFrameWithRetry(int32_t varHash, float value, uint8_t retries = CAN_TX_RETRY_COUNT)
{
    unsigned char data[8];
    writeInt32BigEndian(varHash, &data[0]);
    writeFloat32BigEndian(value, &data[4]);
    
    for (uint8_t attempt = 0; attempt <= retries; ++attempt)
    {
        byte ret = CAN.sendMsgBuf(CAN_ID_VARIABLE_SET, 0, 8, data);
        if (ret == CAN_SENDMSGTIMEOUT || ret == CAN_OK)
        {
            canTxSuccessCount++;
            return true;
        }
        
        // Retry with delay (except on last attempt)
        if (attempt < retries)
        {
            delay(CAN_TX_RETRY_DELAY_MS);
        }
    }
    
    // All retries failed
    canTxFailureCount++;
    return false;
}

// Legacy wrapper for backward compatibility
static inline void sendVariableSetFrame(int32_t varHash, float value)
{
    sendVariableSetFrameWithRetry(varHash, value, 0);  // No retries for inline function (use WithRetry for critical frames)
}

static inline bool sendVariableRequest(int32_t varHash)
{
    unsigned char data[4];
    writeInt32BigEndian(varHash, &data[0]);
    byte ret = CAN.sendMsgBuf(CAN_ID_VAR_REQUEST, 0, 4, data);
    bool success = (ret == CAN_SENDMSGTIMEOUT || ret == CAN_OK);
    if (success) canTxSuccessCount++; else canTxFailureCount++;
    return success;
}

// Function call request functions (EPIC protocol)
// Function request frame format: [Function ID (uint16)] [Arg1 (float32)] [Arg2 (int16) optional]

// Send function call with no arguments (DLC = 6 bytes: 2 bytes ID + 4 bytes Arg1 = 0.0)
static inline bool sendFunctionRequest0(uint16_t funcId)
{
    unsigned char data[6];
    // Function ID (bytes 0-1, big-endian)
    data[0] = (unsigned char)((funcId >> 8) & 0xFF);
    data[1] = (unsigned char)(funcId & 0xFF);
    // Argument 1 = 0.0 (bytes 2-5)
    writeFloat32BigEndian(0.0, &data[2]);
    byte ret = CAN.sendMsgBuf(CAN_ID_FUNCTION_REQUEST, 0, 6, data);
    return (ret == CAN_SENDMSGTIMEOUT || ret == CAN_OK);
}

// Send function call with 1 argument (DLC = 6 bytes: 2 bytes ID + 4 bytes Arg1)
static inline bool sendFunctionRequest1(uint16_t funcId, float arg1)
{
    unsigned char data[6];
    // Function ID (bytes 0-1, big-endian)
    data[0] = (unsigned char)((funcId >> 8) & 0xFF);
    data[1] = (unsigned char)(funcId & 0xFF);
    // Argument 1 (bytes 2-5, float32 big-endian)
    writeFloat32BigEndian(arg1, &data[2]);
    byte ret = CAN.sendMsgBuf(CAN_ID_FUNCTION_REQUEST, 0, 6, data);
    return (ret == CAN_SENDMSGTIMEOUT || ret == CAN_OK);
}

// Send function call with 2 arguments (DLC = 8 bytes: 2 bytes ID + 4 bytes Arg1 + 2 bytes Arg2)
static inline bool sendFunctionRequest2(uint16_t funcId, float arg1, int16_t arg2)
{
    unsigned char data[8];
    // Function ID (bytes 0-1, big-endian)
    data[0] = (unsigned char)((funcId >> 8) & 0xFF);
    data[1] = (unsigned char)(funcId & 0xFF);
    // Argument 1 (bytes 2-5, float32 big-endian)
    writeFloat32BigEndian(arg1, &data[2]);
    // Argument 2 (bytes 6-7, int16 big-endian)
    data[6] = (unsigned char)((arg2 >> 8) & 0xFF);
    data[7] = (unsigned char)(arg2 & 0xFF);
    byte ret = CAN.sendMsgBuf(CAN_ID_FUNCTION_REQUEST, 0, 8, data);
    return (ret == CAN_SENDMSGTIMEOUT || ret == CAN_OK);
}

// Helper functions for common ECU function calls
// These are convenience wrappers around the base function call functions
// Common function IDs from EPIC protocol specification:

#define FUNC_ID_setFuelAdd           1   // Arg1: fuel adjustment (%)
#define FUNC_ID_setFuelMult           2   // Arg1: fuel multiplier
#define FUNC_ID_setTimingAdd          3   // Arg1: timing adjustment (deg)
#define FUNC_ID_setTimingMult         4   // Arg1: timing multiplier
#define FUNC_ID_setBoostTargetAdd     5   // Arg1: boost target adder (kPa)
#define FUNC_ID_setBoostTargetMult    6   // Arg1: boost target multiplier
#define FUNC_ID_setBoostDutyAdd       7   // Arg1: boost duty cycle adder (%)
#define FUNC_ID_setIdleAdd            8   // Arg1: idle position adder (%)
#define FUNC_ID_setIdleRpm            9   // Arg1: idle RPM target
#define FUNC_ID_getEtbTarget         10   // No args, returns ETB target (%)
#define FUNC_ID_setEtbAdd            11   // Arg1: ETB position adder (%)
#define FUNC_ID_setEwgAdd            12   // Arg1: electronic wastegate adder (%)
#define FUNC_ID_setEtbDisabled        13   // Arg1: disable ETB (0=enable, 1=disable)
#define FUNC_ID_setIgnDisabled       14   // Arg1: disable ignition (0=enable, 1=disable)
#define FUNC_ID_setFuelDisabled      15   // Arg1: disable fuel (0=enable, 1=disable)
#define FUNC_ID_getGpPwm            22   // Arg1: index, returns GPPWM output (%)
#define FUNC_ID_selfStimulateRPM      28   // Arg1: trigger simulator RPM
#define FUNC_ID_getIdlePosition      32   // No args, returns idle position (%)
#define FUNC_ID_getTorque            33   // No args, returns torque estimate (Nm)
#define FUNC_ID_getEngineState       36   // No args, returns engine state (0=stopped, 1=cranking, 2=running)
#define FUNC_ID_setLuaGauge          38   // Arg1: value, Arg2: index

// Convenience wrappers (examples - add more as needed)
static inline bool callSetFuelAdd(float fuelAdjustmentPercent)
{
    return sendFunctionRequest1(FUNC_ID_setFuelAdd, fuelAdjustmentPercent);
}

static inline bool callSetIdleRpm(float idleRpm)
{
    return sendFunctionRequest1(FUNC_ID_setIdleRpm, idleRpm);
}

static inline bool callGetEtbTarget(void)
{
    return sendFunctionRequest0(FUNC_ID_getEtbTarget);
}

static inline bool callGetEngineState(void)
{
    return sendFunctionRequest0(FUNC_ID_getEngineState);
}

static inline bool callSetLuaGauge(float value, int16_t index)
{
    return sendFunctionRequest2(FUNC_ID_setLuaGauge, value, index);
}


void setup()
{
    Serial.begin(115200);
    while(!Serial);
    
    // Load configuration from EEPROM (if enabled)
    #if ENABLE_EEPROM_CONFIG
    if (isEepromConfigValid())
    {
        uint8_t loadedCanId = loadEcuCanId();
        // Note: ECU_CAN_ID is a #define, so we can't change it at runtime
        // But we can validate and warn if mismatch
        if (loadedCanId != ECU_CAN_ID)
        {
            Serial.print("EEPROM config: ECU_CAN_ID=");
            Serial.print(loadedCanId);
            Serial.print(" but code compiled with ECU_CAN_ID=");
            Serial.println(ECU_CAN_ID);
        }
        Serial.println("Configuration loaded from EEPROM");
    }
    else
    {
        Serial.println("No valid EEPROM configuration found (using defaults)");
    }
    #endif
    
    // Load ADC calibration (if enabled)
    #if ENABLE_ADC_CALIBRATION
    loadAdcCalibration();
    Serial.println("ADC calibration loaded");
    #endif
    
    while (CAN_OK != CAN.begin(CAN_500KBPS))    // init can bus : baudrate = 500k
    {
        Serial.println("CAN BUS FAIL!");
        delay(100);
    }
    Serial.println("CAN BUS OK!");

    // Configure analog inputs A0-A15 with internal pullup enabled
    // Note: Pullup will pull floating pins to ~5V (affects ADC readings)
    for (uint8_t i = 0; i < 16; ++i)
    {
        pinMode(A0 + i, INPUT_PULLUP);
    }
    
    // Configure digital button inputs D20-D34
    for (uint8_t pin = 20; pin <= 34; ++pin)
    {
        pinMode(pin, INPUT_PULLUP);
    }
    
    // Configure digital outputs D35-D49
    for (uint8_t pin = 35; pin <= 49; ++pin)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);  // Initialize to safe state (LOW)
    }
    
    // Configure PWM outputs D2-D8, D10-D13, D44-D46
    // Pins are initialized as OUTPUT, PWM is enabled via analogWrite() later
    for (uint8_t i = 0; i < 14; ++i)
    {
        pinMode(PWM_PINS[i], OUTPUT);
        analogWrite(PWM_PINS[i], 0);  // Initialize to 0% duty cycle (safe state)
    }
    
    Serial.println("Pin initialization complete");
    
    // Check if digital output hash is configured
    if (VAR_HASH_D35_D49 == 0)
    {
        Serial.println("WARNING: VAR_HASH_D35_D49 not set - digital outputs disabled");
    }
    
    // Check if PWM hashes are configured
    bool pwmConfigured = false;
    for (uint8_t i = 0; i < 14; ++i)
    {
        if (VAR_HASH_PWM[i] != 0)
        {
            pwmConfigured = true;
            break;
        }
    }
    if (!pwmConfigured)
    {
        Serial.println("WARNING: VAR_HASH_PWM[] not set - PWM outputs disabled");
    }
    
    // Initialize previous values for change detection
    // Read all analog inputs once to establish baseline (will trigger first transmission)
    for (uint8_t i = 0; i < 16; ++i)
    {
        previousAnalogValues[i] = (float)analogRead(A0 + i);
    }
    
    // Initialize digital input baseline
    uint16_t bits = 0;
    for (uint8_t pin = 20; pin <= 34; ++pin)
    {
        uint8_t bitIndex = (uint8_t)(pin - 20);
        if (digitalRead(pin) == LOW)
        {
            bits |= (uint16_t)(1u << bitIndex);
        }
    }
    previousDigitalBits = bits;
    
    Serial.println("Performance optimization: Change detection enabled");
    Serial.print("Analog threshold: ");
    Serial.print(ANALOG_CHANGE_THRESHOLD);
    Serial.println(" ADC counts");
    Serial.print("Analog heartbeat: ");
    Serial.print(ANALOG_HEARTBEAT_INTERVAL_MS);
    Serial.println(" ms");
    
    // Initialize interrupt counters (if enabled)
    #if ENABLE_INTERRUPT_COUNTERS
    pinMode(COUNTER_PIN_1, INPUT_PULLUP);
    pinMode(COUNTER_PIN_2, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(COUNTER_PIN_1), counter1ISR, FALLING);  // INT3
    attachInterrupt(digitalPinToInterrupt(COUNTER_PIN_2), counter2ISR, FALLING);  // INT2
    Serial.println("Interrupt counters enabled on D18, D19");
    #endif
    
    // Feature status summary
    Serial.println("\n=== Feature Status ===");
    Serial.print("Interrupt Counters: ");
    Serial.println(ENABLE_INTERRUPT_COUNTERS ? "ENABLED" : "DISABLED");
    Serial.print("ADC Calibration: ");
    Serial.println(ENABLE_ADC_CALIBRATION ? "ENABLED" : "DISABLED");
    Serial.print("EEPROM Config: ");
    Serial.println(ENABLE_EEPROM_CONFIG ? "ENABLED" : "DISABLED");
    Serial.print("Test Mode: ");
    Serial.println(ENABLE_TEST_MODE ? "ENABLED" : "DISABLED");
    Serial.println("=====================\n");
    
    // Initialize performance statistics collection
    statsStartTimeMs = millis();
    statsStartTxSuccess = canTxSuccessCount;
    statsStartTxFailure = canTxFailureCount;
    statsStartRxCount = canRxCount;
    statsStartRxError = canRxErrorCount;
}


// Testing and validation utilities

// Print comprehensive performance statistics
static void printPerformanceStats(void)
{
    unsigned long elapsed = millis() - statsStartTimeMs;
    if (elapsed == 0) elapsed = 1;  // Avoid division by zero
    
    uint32_t txSuccess = canTxSuccessCount - statsStartTxSuccess;
    uint32_t txFailure = canTxFailureCount - statsStartTxFailure;
    uint32_t rxFrames = canRxCount - statsStartRxCount;
    uint32_t rxErrors = canRxErrorCount - statsStartRxError;
    
    float txRate = (float)txSuccess * 1000.0 / (float)elapsed;
    float rxRate = (float)rxFrames * 1000.0 / (float)elapsed;
    float errorRate = (rxFrames > 0) ? ((float)rxErrors * 100.0 / (float)rxFrames) : 0.0;
    
    Serial.println("\n=== Performance Statistics ===");
    Serial.print("Elapsed time: ");
    Serial.print(elapsed / 1000);
    Serial.println(" seconds");
    Serial.print("CAN TX Success: ");
    Serial.print(txSuccess);
    Serial.print(" (");
    Serial.print(txRate, 1);
    Serial.println(" frames/sec)");
    Serial.print("CAN TX Failures: ");
    Serial.println(txFailure);
    Serial.print("CAN RX Frames: ");
    Serial.print(rxFrames);
    Serial.print(" (");
    Serial.print(rxRate, 1);
    Serial.println(" frames/sec)");
    Serial.print("CAN RX Errors: ");
    Serial.print(rxErrors);
    Serial.print(" (");
    Serial.print(errorRate, 2);
    Serial.println("%)");
    Serial.print("ECU Communication: ");
    Serial.println(ecuCommunicationLost ? "LOST" : "OK");
    if (lastEcuResponseMs > 0)
    {
        Serial.print("Last ECU response: ");
        Serial.print((millis() - lastEcuResponseMs) / 1000);
        Serial.println(" seconds ago");
    }
    Serial.println("=============================\n");
}

// Self-test: Verify analog inputs
static void testAnalogInputs(void)
{
    Serial.println("\n=== Analog Input Test ===");
    for (uint8_t i = 0; i < 16; ++i)
    {
        int adc = analogRead(A0 + i);
        float voltage = (adc / 1023.0) * 5.0;
        Serial.print("A");
        Serial.print(i);
        Serial.print(": ADC=");
        Serial.print(adc);
        Serial.print(" (");
        Serial.print(voltage, 2);
        Serial.println("V)");
    }
    Serial.println("========================\n");
}

// Self-test: Verify digital inputs
static void testDigitalInputs(void)
{
    Serial.println("\n=== Digital Input Test ===");
    uint16_t bits = 0;
    for (uint8_t pin = 20; pin <= 34; ++pin)
    {
        int state = digitalRead(pin);
        uint8_t bitIndex = pin - 20;
        if (state == LOW)
        {
            bits |= (1u << bitIndex);
        }
        Serial.print("D");
        Serial.print(pin);
        Serial.print(": ");
        Serial.println(state == LOW ? "LOW (active)" : "HIGH (inactive)");
    }
    Serial.print("Bitfield: 0x");
    Serial.println(bits, HEX);
    Serial.println("========================\n");
}

// Self-test: Test digital outputs (cycling pattern)
static void testDigitalOutputs(void)
{
    Serial.println("Testing digital outputs D35-D49...");
    Serial.println("Watch outputs - should cycle through pattern");
    
    // Pattern: All OFF, All ON, Walking pattern
    for (uint8_t cycle = 0; cycle < 3; ++cycle)
    {
        // All OFF
        for (uint8_t pin = 35; pin <= 49; ++pin)
        {
            digitalWrite(pin, LOW);
        }
        delay(500);
        
        // All ON
        for (uint8_t pin = 35; pin <= 49; ++pin)
        {
            digitalWrite(pin, HIGH);
        }
        delay(500);
        
        // Walking pattern
        for (uint8_t pin = 35; pin <= 49; ++pin)
        {
            for (uint8_t p = 35; p <= 49; ++p)
            {
                digitalWrite(p, (p == pin) ? HIGH : LOW);
            }
            delay(100);
        }
    }
    
    // Return to safe state
    enterSafeMode();
    Serial.println("Digital output test complete - outputs returned to safe state");
}

// Self-test: Test PWM outputs (sweep pattern)
static void testPwmOutputs(void)
{
    Serial.println("Testing PWM outputs...");
    Serial.println("Watch outputs - should sweep from 0% to 100%");
    
    // Sweep each PWM channel from 0% to 100%
    for (uint8_t i = 0; i < 14; ++i)
    {
        Serial.print("Testing PWM D");
        Serial.print(PWM_PINS[i]);
        Serial.println("...");
        
        for (uint8_t duty = 0; duty <= 255; duty += 5)
        {
            analogWrite(PWM_PINS[i], duty);
            delay(20);
        }
        delay(500);
        analogWrite(PWM_PINS[i], 0);
        delay(200);
    }
    
    Serial.println("PWM output test complete - outputs returned to safe state");
}

// Serial command parser (if enabled)
#if ENABLE_SERIAL_COMMANDS
static void processSerialCommand(void)
{
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        cmd.toUpperCase();
        
        if (cmd == "STATS" || cmd == "S")
        {
            printPerformanceStats();
        }
        else if (cmd == "TESTANALOG" || cmd == "TA")
        {
            testAnalogInputs();
        }
        else if (cmd == "TESTDIGITAL" || cmd == "TD")
        {
            testDigitalInputs();
        }
        else if (cmd == "TESTOUTPUTS" || cmd == "TO")
        {
            testDigitalOutputs();
        }
        else if (cmd == "TESTPWM" || cmd == "TP")
        {
            testPwmOutputs();
        }
        else if (cmd == "HELP" || cmd == "H" || cmd == "?")
        {
            Serial.println("\n=== Serial Commands ===");
            Serial.println("STATS (S)     - Print performance statistics");
            Serial.println("TESTANALOG (TA) - Test analog inputs A0-A15");
            Serial.println("TESTDIGITAL (TD) - Test digital inputs D20-D34");
            Serial.println("TESTOUTPUTS (TO) - Test digital outputs D35-D49");
            Serial.println("TESTPWM (TP)  - Test PWM outputs");
            Serial.println("HELP (H, ?)   - Show this help");
            Serial.println("======================\n");
        }
        else if (cmd.length() > 0)
        {
            Serial.print("Unknown command: ");
            Serial.println(cmd);
            Serial.println("Type HELP for available commands");
        }
    }
}
#endif

// Safe mode: Set all outputs to safe states
static void enterSafeMode(void)
{
    // Set all digital outputs to LOW (safe state)
    for (uint8_t pin = 35; pin <= 49; ++pin)
    {
        digitalWrite(pin, LOW);
    }
    
    // Set all PWM outputs to 0% duty cycle (safe state)
    for (uint8_t i = 0; i < 14; ++i)
    {
        analogWrite(PWM_PINS[i], 0);
    }
}

// Process incoming CAN frames (EPIC protocol)
static void processCanFrame(unsigned long canId, unsigned char len, unsigned char* buf)
{
    canRxCount++;  // Increment RX counter
    
    // Enhanced frame validation
    // Validate frame length (EPIC frames should be 8 bytes, except var_request which is 4)
    if (len == 0 || len > 8)
    {
        Serial.print("Invalid frame length: ");
        Serial.println(len);
        canRxErrorCount++;
        return;
    }
    
    // Validate CAN ID range (should be in EPIC protocol range for this ECU)
    // EPIC IDs: 0x700-0x70F, 0x720-0x72F, 0x740-0x74F, 0x760-0x76F, 0x780-0x78F
    unsigned long baseId = canId & 0xFF0;  // Mask to base ID
    unsigned long ecuId = canId & 0x00F;   // Extract ECU ID
    if (ecuId != ECU_CAN_ID || 
        (baseId != 0x700 && baseId != 0x720 && baseId != 0x740 && 
         baseId != 0x760 && baseId != 0x780))
    {
        Serial.print("Invalid CAN ID: 0x");
        Serial.println(canId, HEX);
        canRxErrorCount++;
        return;
    }
    
    // Parse variable_response frames (0x720 + ecuCanId)
    if (IS_VAR_RESPONSE(canId))
    {
        if (len != 8)
        {
            Serial.println("Error: Variable response must be 8 bytes");
            canRxErrorCount++;
            return;
        }
        
        // Update ECU communication watchdog - successful response received
        lastEcuResponseMs = millis();
        if (ecuCommunicationLost)
        {
            ecuCommunicationLost = false;
            Serial.println("ECU communication restored");
        }
        
        // Extract variable hash (bytes 0-3)
        int32_t varHash = readInt32BigEndian(&buf[0]);
        
        // Extract variable value (bytes 4-7)
        float value = readFloat32BigEndian(&buf[4]);
        
        // Handle digital output bitfield (D35-D49)
        if (VAR_HASH_D35_D49 != 0 && varHash == VAR_HASH_D35_D49)
        {
            // Only update outputs if communication is healthy
            if (!ecuCommunicationLost)
            {
                // Unpack 15-bit bitfield and apply to D35-D49
                uint16_t bits = (uint16_t)value;  // Cast float to uint16_t (bitfield)
                
                for (uint8_t pin = 35; pin <= 49; ++pin)
                {
                    uint8_t bitIndex = (uint8_t)(pin - 35);
                    bool state = (bits >> bitIndex) & 1;
                    digitalWrite(pin, state ? HIGH : LOW);
                }
            }
            
            // Optional debug output (comment out for production)
            // Serial.print("Digital outputs updated: 0x");
            // Serial.println(bits, HEX);
        }
        // Handle PWM duty cycle values (0-100% float32)
        else
        {
            // Check if this is a PWM variable response
            for (uint8_t i = 0; i < 14; ++i)
            {
                if (VAR_HASH_PWM[i] != 0 && varHash == VAR_HASH_PWM[i])
                {
                    // Value is duty cycle percentage (0.0-100.0)
                    // Clamp to valid range
                    if (value < 0.0) value = 0.0;
                    if (value > 100.0) value = 100.0;
                    
                    // Only update PWM if communication is healthy
                    if (!ecuCommunicationLost)
                    {
                        // Convert percentage (0-100%) to PWM range (0-255)
                        uint8_t pwmValue = (uint8_t)((value / 100.0) * 255.0);
                        
                        // Apply PWM output
                        analogWrite(PWM_PINS[i], pwmValue);
                    }
                    
                    // Optional debug output (comment out for production)
                    // Serial.print("PWM D");
                    // Serial.print(PWM_PINS[i]);
                    // Serial.print(": ");
                    // Serial.print(value);
                    // Serial.print("% -> ");
                    // Serial.println(pwmValue);
                    
                    return;  // Found and handled, exit
                }
            }
            
            // Unknown variable response (not digital outputs or PWM)
            // Optional debug output (comment out for production)
            // Serial.print("Unknown var response - Hash: ");
            // Serial.print(varHash);
            // Serial.print(", Value: ");
            // Serial.println(value);
        }
    }
    // Parse function_response frames (0x760 + ecuCanId)
    else if (IS_FUNCTION_RESPONSE(canId))
    {
        if (len != 8)
        {
            Serial.println("Error: Function response must be 8 bytes");
            canRxErrorCount++;
            return;
        }
        
        // Update ECU communication watchdog - successful response received
        lastEcuResponseMs = millis();
        if (ecuCommunicationLost)
        {
            ecuCommunicationLost = false;
            Serial.println("ECU communication restored");
        }
        
        // Extract function ID (bytes 0-1, big-endian uint16) - echo of called function
        uint16_t funcId = ((uint16_t)buf[0] << 8) | buf[1];
        
        // Bytes 2-3: Reserved (should be 0x00 0x00, but we skip them)
        
        // Extract return value (bytes 4-7, float32 big-endian)
        // Note: 0.0 indicates error or function doesn't return value
        float returnValue = readFloat32BigEndian(&buf[4]);
        
        // Function response handling
        // Applications can match funcId with pending requests and use returnValue
        // Optional debug output (comment out for production)
        // Serial.print("Function response - ID: ");
        // Serial.print(funcId);
        // Serial.print(", Return: ");
        // Serial.println(returnValue);
        
        // Example: Handle specific function responses here
        // if (funcId == 10) {  // getEtbTarget
        //     // Use returnValue
        // }
    }
    // Parse variable_set frames (if ECU sends variable_set to Mega - optional)
    else if (IS_VAR_SET(canId))
    {
        if (len != 8)
        {
            Serial.println("Error: Variable set must be 8 bytes");
            canRxErrorCount++;
            return;
        }
        
        // Update ECU communication watchdog - successful communication
        lastEcuResponseMs = millis();
        if (ecuCommunicationLost)
        {
            ecuCommunicationLost = false;
            Serial.println("ECU communication restored");
        }
        
        // Handle variable_set from ECU (if needed in future)
        // int32_t varHash = readInt32BigEndian(&buf[0]);
        // float value = readFloat32BigEndian(&buf[4]);
    }
    else
    {
        // Unknown frame type - log for debugging
        Serial.print("Unknown CAN ID: 0x");
        Serial.print(canId, HEX);
        Serial.print(", Length: ");
        Serial.println(len);
        canRxErrorCount++;
    }
}

void loop()
{
    unsigned char len = 0;
    unsigned char buf[8];

    // Process incoming CAN frames
    if(CAN_MSGAVAIL == CAN.checkReceive())            // check if data coming
    {
        CAN.readMsgBuf(&len, buf);    // read data,  len: data length, buf: data buf
        unsigned long canId = CAN.getCanId();
        
        // Process the frame
        processCanFrame(canId, len, buf);
    }

    // Periodic transmission of analog and digital inputs with change detection
    static unsigned long lastTxMs = 0;
    static unsigned long lastDigitalOutputRequestMs = 0;
    static unsigned long lastPwmRequestMs = 0;
    static uint8_t currentPwmChannel = 0;  // Current PWM channel to request (0-13)
    unsigned long nowMs = millis();
    
    // Check if heartbeat interval has elapsed (send all values regardless of change)
    bool heartbeatDue = (nowMs - lastAnalogHeartbeatMs >= ANALOG_HEARTBEAT_INTERVAL_MS);
    
    if (nowMs - lastTxMs >= TRANSMIT_INTERVAL_MS)
    {
        lastTxMs = nowMs;

        // Transmit analog inputs A0..A15 with change detection
        for (uint8_t i = 0; i < 16; ++i)
        {
            int adc = analogRead(A0 + i);
            float value = (float)adc;
            
            // Apply ADC calibration if enabled
            #if ENABLE_ADC_CALIBRATION
            value = applyAdcCalibration(i, value);
            #endif
            
            // Transmit if: change detected OR heartbeat due
            float change = (value > previousAnalogValues[i]) ? 
                          (value - previousAnalogValues[i]) : 
                          (previousAnalogValues[i] - value);
            
            if (change >= ANALOG_CHANGE_THRESHOLD || heartbeatDue)
            {
                sendVariableSetFrame(VAR_HASH_ANALOG[i], value);
                previousAnalogValues[i] = value;  // Update previous value
                
                // Optional debug output (comment out for production)
                // Serial.print("Analog A");
                // Serial.print(i);
                // Serial.print(": ");
                // Serial.print(value);
                // if (heartbeatDue) Serial.print(" (heartbeat)");
                // Serial.println();
            }
        }
        
        // Update heartbeat timestamp if heartbeat was sent
        if (heartbeatDue)
        {
            lastAnalogHeartbeatMs = nowMs;
        }

        // Pack D20..D34 into 15-bit field (bit0=D20 ... bit14=D34)
        // Inverted logic: LOW (grounded) = 1, HIGH (pullup active/unconnected) = 0
        uint16_t bits = 0;
        for (uint8_t pin = 20; pin <= 34; ++pin)
        {
            uint8_t bitIndex = (uint8_t)(pin - 20);
            int state = digitalRead(pin);
            if (state == LOW)  // Grounded = report as 1
            {
                bits |= (uint16_t)(1u << bitIndex);
            }
        }
        
        // Transmit digital inputs only if state changed (change detection)
        if (bits != previousDigitalBits || !DIGITAL_CHANGE_ONLY)
        {
            sendVariableSetFrame(VAR_HASH_D20_D34, (float)bits);
            previousDigitalBits = bits;  // Update previous state
            
            // Optional debug output (comment out for production)
            // Serial.print("Digital inputs: 0x");
            // Serial.println(bits, HEX);
        }
    }
    
    // Periodic variable_request for digital outputs (D35-D49)
    if (VAR_HASH_D35_D49 != 0)
    {
        if (nowMs - lastDigitalOutputRequestMs >= DIGITAL_OUTPUT_POLL_INTERVAL_MS)
        {
            lastDigitalOutputRequestMs = nowMs;
            
            // Request digital output bitfield from ECU
            bool success = sendVariableRequest(VAR_HASH_D35_D49);
            if (!success)
            {
                Serial.println("Error: Failed to send variable_request for digital outputs");
            }
        }
    }
    
    // Periodic variable_request for PWM outputs (14 channels, staggered)
    // Request one channel per polling interval to spread CAN traffic
    if (nowMs - lastPwmRequestMs >= PWM_POLL_INTERVAL_MS)
    {
        lastPwmRequestMs = nowMs;
        
        // Find next configured PWM channel to request
        bool pwmRequestSent = false;
        uint8_t attempts = 0;
        
        while (!pwmRequestSent && attempts < 14)
        {
            // Check if current channel has a valid hash
            if (VAR_HASH_PWM[currentPwmChannel] != 0)
            {
                // Request PWM duty cycle for this channel
                bool success = sendVariableRequest(VAR_HASH_PWM[currentPwmChannel]);
                if (!success)
                {
                    Serial.print("Error: Failed to send variable_request for PWM channel ");
                    Serial.print(currentPwmChannel);
                    Serial.print(" (D");
                    Serial.print(PWM_PINS[currentPwmChannel]);
                    Serial.println(")");
                }
                pwmRequestSent = true;
            }
            
            // Move to next channel (round-robin)
            currentPwmChannel = (currentPwmChannel + 1) % 14;
            attempts++;
        }
    }
    
    // Serial command processing (if enabled)
    #if ENABLE_SERIAL_COMMANDS
    processSerialCommand();
    #endif
    
    // Performance statistics reporting (if enabled)
    #if PERFORMANCE_STATS_INTERVAL_MS > 0
    static unsigned long lastStatsPrintMs = 0;
    if (nowMs - lastStatsPrintMs >= PERFORMANCE_STATS_INTERVAL_MS)
    {
        lastStatsPrintMs = nowMs;
        printPerformanceStats();
    }
    #endif
    
    // Test mode: I/O pattern testing (if enabled)
    #if ENABLE_TEST_MODE
    static unsigned long testPatternMs = 0;
    static uint8_t testPatternState = 0;
    
    if (!testModeActive)
    {
        testModeActive = true;
        testModeStartMs = nowMs;
        Serial.println("TEST MODE ACTIVE - Running I/O test patterns");
        Serial.println("Set ENABLE_TEST_MODE=false to disable");
    }
    
    // Simple test pattern: cycle digital outputs every 2 seconds
    if (nowMs - testPatternMs >= 2000)
    {
        testPatternMs = nowMs;
        uint16_t pattern = (testPatternState & 1) ? 0xFFFF : 0x0000;
        testPatternState++;
        
        // Apply pattern to digital outputs (if not in safe mode)
        if (!ecuCommunicationLost)
        {
            for (uint8_t pin = 35; pin <= 49; ++pin)
            {
                uint8_t bitIndex = pin - 35;
                digitalWrite(pin, (pattern >> bitIndex) & 1 ? HIGH : LOW);
            }
        }
        
        // Cycle PWM outputs (0%, 25%, 50%, 75%, 100%)
        uint8_t pwmDuty = ((testPatternState / 2) % 5) * 51;  // 0, 51, 102, 153, 204
        for (uint8_t i = 0; i < 14; ++i)
        {
            if (!ecuCommunicationLost)
            {
                analogWrite(PWM_PINS[i], pwmDuty);
            }
        }
    }
    #endif
    
    // Interrupt Counter Reporting (if enabled)
    #if ENABLE_INTERRUPT_COUNTERS
    static unsigned long lastCounterReportMs = 0;  // Track last counter report time
    if (nowMs - lastCounterReportMs >= COUNTER_REPORT_INTERVAL_MS)
    {
        lastCounterReportMs = nowMs;
        
        // Calculate delta counts (since last report)
        uint32_t delta1 = interruptCounter1 - lastReportedCounter1;
        uint32_t delta2 = interruptCounter2 - lastReportedCounter2;
        
        // Update last reported values
        lastReportedCounter1 = interruptCounter1;
        lastReportedCounter2 = interruptCounter2;
        
        // Transmit counter deltas (if configured and non-zero)
        if (VAR_HASH_COUNTER1 != 0 && delta1 > 0)
        {
            sendVariableSetFrame(VAR_HASH_COUNTER1, (float)delta1);
        }
        if (VAR_HASH_COUNTER2 != 0 && delta2 > 0)
        {
            sendVariableSetFrame(VAR_HASH_COUNTER2, (float)delta2);
        }
    }
    #endif
    
    // ECU Communication Watchdog - Check for communication loss
    unsigned long nowMsComm = millis();
    if (lastEcuResponseMs > 0)  // Only check after first response received
    {
        unsigned long timeSinceLastResponse = nowMsComm - lastEcuResponseMs;
        
        if (timeSinceLastResponse > ECU_COMM_TIMEOUT_MS && !ecuCommunicationLost)
        {
            // ECU communication lost - enter safe mode
            ecuCommunicationLost = true;
            Serial.println("WARNING: ECU communication lost - entering safe mode");
            enterSafeMode();
            
            // Print diagnostics
            Serial.println("CAN Diagnostics:");
            Serial.print("  TX Success: ");
            Serial.println(canTxSuccessCount);
            Serial.print("  TX Failures: ");
            Serial.println(canTxFailureCount);
            Serial.print("  RX Frames: ");
            Serial.println(canRxCount);
            Serial.print("  RX Errors: ");
            Serial.println(canRxErrorCount);
        }
    }
    else if (nowMsComm > 5000)
    {
        // No response received within 5 seconds of boot - initial warning
        static bool initialWarningPrinted = false;
        if (!initialWarningPrinted)
        {
            Serial.println("WARNING: No ECU response received yet (check CAN bus connection)");
            initialWarningPrinted = true;
        }
    }
}

// END FILE
