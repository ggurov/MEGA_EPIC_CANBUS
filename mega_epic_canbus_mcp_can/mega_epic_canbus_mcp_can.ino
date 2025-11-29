/*
  MEGA_EPIC_CANBUS - Full Port for MCP_CAN Library
  -------------------------------------------------
  Arduino Mega2560 + Seeed Studio CAN-BUS Shield V2.0
  
  Ported from PWFusion_MCP2515 library to MCP_CAN library
  Original by: Gennady Gurov
  Ported by: Claude (Full Feature Port)
*/

#include <SPI.h>
#include <stdint.h>
#include <math.h>
#include <mcp_can.h>
#include <string.h>
#include "nmea_parser.h"

// ===== CONFIGURATION =====
#define SPI_CS_PIN  9
#define MCP2515_INT_PIN  2

MCP_CAN CAN(SPI_CS_PIN);

// ECU and EPIC CAN configuration
#define ECU_CAN_ID  1
#define CAN_ID_VAR_REQUEST        (0x700 + ECU_CAN_ID)
#define CAN_ID_VAR_RESPONSE       (0x720 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_REQUEST   (0x740 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_RESPONSE  (0x760 + ECU_CAN_ID)
#define CAN_ID_VARIABLE_SET       (0x780 + ECU_CAN_ID)

// Timing
#define SLOW_OUT_REQUEST_INTERVAL_MS 25
#define TX_INTERVAL_FAST_MS    25
#define TX_INTERVAL_SLOW_MS    500
#define TX_READ_INTERVAL_MS    10

// Thresholds
#define TX_ANALOG_THRESHOLD    2.0f
#define TX_VSS_THRESHOLD       0.1f
#define GPS_FLOAT_THRESHOLD    0.001f

// EPIC variable hashes
const int32_t VAR_HASH_ANALOG[16] = {
    595545759, 595545760, 595545761, 595545762,
    595545763, 595545764, 595545765, 595545766,
    595545767, 595545768, -1821826352, -1821826351,
   -1821826350, -1821826349, -1821826348, -1821826347
};

const int32_t VAR_HASH_D22_D37 = 2138825443;
const int32_t VAR_HASH_OUT_SLOW = 1430780106;

// GPS hashes
const int32_t VAR_HASH_GPS_HMSD_PACKED = 703958849;
const int32_t VAR_HASH_GPS_MYQSAT_PACKED = -1519914092;
const int32_t VAR_HASH_GPS_ACCURACY = -1489698215;
const int32_t VAR_HASH_GPS_ALTITUDE = -2100224086;
const int32_t VAR_HASH_GPS_COURSE = 1842893663;
const int32_t VAR_HASH_GPS_LATITUDE = 1524934922;
const int32_t VAR_HASH_GPS_LONGITUDE = -809214087;
const int32_t VAR_HASH_GPS_SPEED = -1486968225;

// VSS configuration
#define VSS_FRONT_LEFT_PIN   18
#define VSS_FRONT_RIGHT_PIN  19
#define VSS_REAR_LEFT_PIN    20
#define VSS_REAR_RIGHT_PIN   21
#define VSS_CALC_INTERVAL_MS 25
#define VSS_ENABLE_PULLUP    1

const int32_t VAR_HASH_VSS_FRONT_LEFT  = -1645222329;
const int32_t VAR_HASH_VSS_FRONT_RIGHT = 1549498074;
const int32_t VAR_HASH_VSS_REAR_LEFT   = 768443592;
const int32_t VAR_HASH_VSS_REAR_RIGHT  = -403905157;

// GPIO pins
const uint8_t SLOW_GPIO_PINS[8] = {39, 40, 41, 42, 43, 47, 48, 49};
const uint8_t PWM_OUTPUT_PINS[10] = {3, 5, 6, 7, 8, 11, 12, 44, 45, 46};

// GPS
#define GPS_SERIAL Serial2
#define GPS_BAUD_RATE 115200

bool gpsEnabled = false;
bool gpsInitialized = false;
static GPSData gpsData = {0};
static GPSData lastTransmittedGpsData = {0};

// ===== DATA STRUCTURES =====
struct VSSChannel {
    volatile uint32_t edgeCount;
    uint32_t lastCount;
    unsigned long lastCalcTime;
    float pulsesPerSecond;
};

VSSChannel vssChannels[4] = {
    {0, 0, 0, 0.0f}, {0, 0, 0, 0.0f},
    {0, 0, 0, 0.0f}, {0, 0, 0, 0.0f}
};

#define TX_STATE_CHANGED  0
#define TX_STATE_STABLE   1

struct TxChannelState {
    float lastTransmittedValue;
    unsigned long lastTxTime;
    bool hasChanged;
    uint8_t state;
};

TxChannelState analogTxState[16];
TxChannelState digitalTxState;
TxChannelState vssTxState[4];
TxChannelState gpsTxState[8];

static float currentAnalogValues[16] = {0};
static uint16_t currentDigitalBits = 0;
static float currentVssValues[4] = {0};

// ===== HELPER FUNCTIONS =====
static inline void writeInt32BigEndian(int32_t value, unsigned char* out) {
    out[0] = (unsigned char)((value >> 24) & 0xFF);
    out[1] = (unsigned char)((value >> 16) & 0xFF);
    out[2] = (unsigned char)((value >> 8) & 0xFF);
    out[3] = (unsigned char)(value & 0xFF);
}

static inline void writeFloat32BigEndian(float value, unsigned char* out) {
    union { float f; uint32_t u; } conv;
    conv.f = value;
    out[0] = (unsigned char)((conv.u >> 24) & 0xFF);
    out[1] = (unsigned char)((conv.u >> 16) & 0xFF);
    out[2] = (unsigned char)((conv.u >> 8) & 0xFF);
    out[3] = (unsigned char)(conv.u & 0xFF);
}

static inline int32_t readInt32BigEndian(const unsigned char* in) {
    int32_t value = 0;
    value |= ((int32_t)in[0] << 24);
    value |= ((int32_t)in[1] << 16);
    value |= ((int32_t)in[2] << 8);
    value |= ((int32_t)in[3]);
    return value;
}

static inline float readFloat32BigEndian(const unsigned char* in) {
    union { float f; uint32_t u; } conv;
    conv.u =  ((uint32_t)in[0] << 24)
            | ((uint32_t)in[1] << 16)
            | ((uint32_t)in[2] << 8)
            | ((uint32_t)in[3]);
    return conv.f;
}

// ===== CAN SEND FUNCTIONS =====
static inline void sendVariableSetFrame(int32_t varHash, float value) {
    byte data[8];
    writeInt32BigEndian(varHash, &data[0]);
    writeFloat32BigEndian(value, &data[4]);
    CAN.sendMsgBuf(CAN_ID_VARIABLE_SET, 0, 8, data);
}

static inline void sendVariableSetFrameU32(int32_t varHash, uint32_t value) {
    byte data[8];
    writeInt32BigEndian(varHash, &data[0]);
    writeInt32BigEndian(value, &data[4]);
    CAN.sendMsgBuf(CAN_ID_VARIABLE_SET, 0, 8, data);
}

static inline void sendVariableRequestFrame(int32_t varHash) {
    byte data[4];
    writeInt32BigEndian(varHash, &data[0]);
    CAN.sendMsgBuf(CAN_ID_VAR_REQUEST, 0, 4, data);
}

// ===== VSS ISRs =====
void vssFrontLeftISR() {
    if (vssChannels[0].edgeCount < 0xFFFFFFFE) {
        vssChannels[0].edgeCount++;
    }
}

void vssFrontRightISR() {
    if (vssChannels[1].edgeCount < 0xFFFFFFFE) {
        vssChannels[1].edgeCount++;
    }
}

void vssRearLeftISR() {
    if (vssChannels[2].edgeCount < 0xFFFFFFFE) {
        vssChannels[2].edgeCount++;
    }
}

void vssRearRightISR() {
    if (vssChannels[3].edgeCount < 0xFFFFFFFE) {
        vssChannels[3].edgeCount++;
    }
}

// ===== INPUT READING =====
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
        if (state == LOW) {
            *bits |= (uint16_t)(1u << bitIndex);
        }
    }
}

void readVSSInputs(float* values) {
    for (uint8_t i = 0; i < 4; ++i) {
        values[i] = vssChannels[i].pulsesPerSecond;
    }
}

void calculateVSSRates() {
    unsigned long nowMs = millis();
    
    for (uint8_t i = 0; i < 4; ++i) {
        if ((nowMs - vssChannels[i].lastCalcTime) >= VSS_CALC_INTERVAL_MS) {
            noInterrupts();
            uint32_t currentCount = vssChannels[i].edgeCount;
            interrupts();
            
            uint32_t deltaCount = currentCount - vssChannels[i].lastCount;
            unsigned long deltaTime = nowMs - vssChannels[i].lastCalcTime;
            
            if (deltaTime > 0) {
                vssChannels[i].pulsesPerSecond = (float)deltaCount * 1000.0f / (float)deltaTime;
            }
            
            vssChannels[i].lastCount = currentCount;
            vssChannels[i].lastCalcTime = nowMs;
        }
    }
}

// ===== CHANGE DETECTION =====
bool hasAnalogChanged(uint8_t channel, float newValue) {
    float diff = newValue - analogTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff;
    return (diff >= TX_ANALOG_THRESHOLD);
}

bool hasDigitalChanged(uint16_t newBits) {
    return (newBits != (uint16_t)digitalTxState.lastTransmittedValue);
}

bool hasVSSChanged(uint8_t channel, float newValue) {
    float diff = newValue - vssTxState[channel].lastTransmittedValue;
    if (diff < 0) diff = -diff;
    return (diff >= TX_VSS_THRESHOLD);
}

void updateTxState(TxChannelState* state, bool changed, unsigned long nowMs) {
    if (changed) {
        state->hasChanged = true;
        state->state = TX_STATE_CHANGED;
    } else {
        state->hasChanged = false;
        if (state->state == TX_STATE_CHANGED && 
            (nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS) {
            state->state = TX_STATE_STABLE;
        }
    }
}

bool shouldTransmit(TxChannelState* state, unsigned long nowMs) {
    if (state->hasChanged) {
        return ((nowMs - state->lastTxTime) >= TX_INTERVAL_FAST_MS);
    } else {
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

// ===== GPS FUNCTIONS =====
static inline uint32_t packGPSHMSD(uint8_t hours, uint8_t minutes, uint8_t seconds, uint8_t days) {
    uint32_t packed = ((uint32_t)hours) |
                      ((uint32_t)minutes << 8) |
                      ((uint32_t)seconds << 16) |
                      ((uint32_t)days << 24);
    return packed;
}

static inline uint32_t packGPSMYQSAT(uint8_t months, uint8_t years, uint8_t quality, uint8_t satellites) {
    uint32_t packed = ((uint32_t)months) |
                      ((uint32_t)years << 8) |
                      ((uint32_t)quality << 16) |
                      ((uint32_t)satellites << 24);
    return packed;
}

static bool readGPSData() {
    static unsigned long lastDataMs = 0;
    unsigned long nowMs = millis();
    bool dataReceived = false;
    
    while (GPS_SERIAL.available() > 0) {
        char c = GPS_SERIAL.read();
        lastDataMs = nowMs;
        
        if (nmeaParserProcessChar(c, &gpsData)) {
            dataReceived = true;
            
            if (!gpsEnabled) {
                gpsEnabled = true;
            }
        }
    }
    
    if (gpsEnabled && (nowMs - lastDataMs) > 2000) {
        gpsEnabled = false;
        gpsData.dataValid = false;
        gpsData.hasFix = false;
    }
    
    return dataReceived;
}

bool hasGPSChanged(uint8_t index) {
    switch(index) {
        case 0: {
            float current = packGPSHMSD(gpsData.hours, gpsData.minutes, gpsData.seconds, gpsData.days);
            float last = packGPSHMSD(lastTransmittedGpsData.hours, lastTransmittedGpsData.minutes,
                                    lastTransmittedGpsData.seconds, lastTransmittedGpsData.days);
            return (current != last);
        }
        case 1: {
            float current = packGPSMYQSAT(gpsData.months, gpsData.years, gpsData.quality, gpsData.satellites);
            float last = packGPSMYQSAT(lastTransmittedGpsData.months, lastTransmittedGpsData.years,
                                      lastTransmittedGpsData.quality, lastTransmittedGpsData.satellites);
            return (current != last);
        }
        case 2:
            return (fabs(gpsData.accuracy - lastTransmittedGpsData.accuracy) > GPS_FLOAT_THRESHOLD);
        case 3:
            return (fabs(gpsData.altitude - lastTransmittedGpsData.altitude) > GPS_FLOAT_THRESHOLD);
        case 4:
            return (fabs(gpsData.course - lastTransmittedGpsData.course) > GPS_FLOAT_THRESHOLD);
        case 5:
            return (fabs(gpsData.latitude - lastTransmittedGpsData.latitude) > GPS_FLOAT_THRESHOLD);
        case 6:
            return (fabs(gpsData.longitude - lastTransmittedGpsData.longitude) > GPS_FLOAT_THRESHOLD);
        case 7:
            return (fabs(gpsData.speed - lastTransmittedGpsData.speed) > GPS_FLOAT_THRESHOLD);
        default:
            return false;
    }
}

void transmitGPSIfNeeded() {
    if (!gpsEnabled || !gpsData.dataValid) {
        return;
    }
    
    unsigned long nowMs = millis();
    
    // HMSD
    if (hasGPSChanged(0)) {
        updateTxState(&gpsTxState[0], true, nowMs);
    }
    if (shouldTransmit(&gpsTxState[0], nowMs)) {
        uint32_t value = packGPSHMSD(gpsData.hours, gpsData.minutes, gpsData.seconds, gpsData.days);
        sendVariableSetFrameU32(VAR_HASH_GPS_HMSD_PACKED, value);
        gpsTxState[0].lastTxTime = nowMs;
        lastTransmittedGpsData.hours = gpsData.hours;
        lastTransmittedGpsData.minutes = gpsData.minutes;
        lastTransmittedGpsData.seconds = gpsData.seconds;
        lastTransmittedGpsData.days = gpsData.days;
    }
    
    // MYQSAT
    if (hasGPSChanged(1)) {
        updateTxState(&gpsTxState[1], true, nowMs);
    }
    if (shouldTransmit(&gpsTxState[1], nowMs)) {
        uint32_t value = packGPSMYQSAT(gpsData.months, gpsData.years, gpsData.quality, gpsData.satellites);
        sendVariableSetFrameU32(VAR_HASH_GPS_MYQSAT_PACKED, value);
        gpsTxState[1].lastTxTime = nowMs;
        lastTransmittedGpsData.months = gpsData.months;
        lastTransmittedGpsData.years = gpsData.years;
        lastTransmittedGpsData.quality = gpsData.quality;
        lastTransmittedGpsData.satellites = gpsData.satellites;
    }
    
    if (gpsData.hasFix) {
        // Accuracy
        if (hasGPSChanged(2)) {
            updateTxState(&gpsTxState[2], true, nowMs);
        }
        if (shouldTransmit(&gpsTxState[2], nowMs)) {
            sendVariableSetFrame(VAR_HASH_GPS_ACCURACY, gpsData.accuracy);
            gpsTxState[2].lastTxTime = nowMs;
            lastTransmittedGpsData.accuracy = gpsData.accuracy;
        }
        
        // Altitude
        if (hasGPSChanged(3)) {
            updateTxState(&gpsTxState[3], true, nowMs);
        }
        if (shouldTransmit(&gpsTxState[3], nowMs)) {
            sendVariableSetFrame(VAR_HASH_GPS_ALTITUDE, gpsData.altitude);
            gpsTxState[3].lastTxTime = nowMs;
            lastTransmittedGpsData.altitude = gpsData.altitude;
        }
        
        // Course
        if (hasGPSChanged(4)) {
            updateTxState(&gpsTxState[4], true, nowMs);
        }
        if (shouldTransmit(&gpsTxState[4], nowMs)) {
            sendVariableSetFrame(VAR_HASH_GPS_COURSE, gpsData.course);
            gpsTxState[4].lastTxTime = nowMs;
            lastTransmittedGpsData.course = gpsData.course;
        }
        
        // Latitude
        if (hasGPSChanged(5)) {
            updateTxState(&gpsTxState[5], true, nowMs);
        }
        if (shouldTransmit(&gpsTxState[5], nowMs)) {
            sendVariableSetFrame(VAR_HASH_GPS_LATITUDE, gpsData.latitude);
            gpsTxState[5].lastTxTime = nowMs;
            lastTransmittedGpsData.latitude = gpsData.latitude;
        }
        
        // Longitude
        if (hasGPSChanged(6)) {
            updateTxState(&gpsTxState[6], true, nowMs);
        }
        if (shouldTransmit(&gpsTxState[6], nowMs)) {
            sendVariableSetFrame(VAR_HASH_GPS_LONGITUDE, gpsData.longitude);
            gpsTxState[6].lastTxTime = nowMs;
            lastTransmittedGpsData.longitude = gpsData.longitude;
        }
        
        // Speed
        if (hasGPSChanged(7)) {
            updateTxState(&gpsTxState[7], true, nowMs);
        }
        if (shouldTransmit(&gpsTxState[7], nowMs)) {
            sendVariableSetFrame(VAR_HASH_GPS_SPEED, gpsData.speed);
            gpsTxState[7].lastTxTime = nowMs;
            lastTransmittedGpsData.speed = gpsData.speed;
        }
    }
}

// ===== SETUP =====
void setup() {
    Serial.begin(115200);
    while(!Serial);
    
    Serial.println(F("=== MEGA_EPIC_CANBUS Full Port ==="));
    Serial.println(F("MCP_CAN Library Version"));
    
    // Initialize CAN
    if(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ) == CAN_OK) {
        Serial.println(F("CAN: Init OK"));
    } else {
        Serial.println(F("CAN: Init FAILED"));
        while(1);
    }
    CAN.setMode(MCP_NORMAL);
    delay(10);
    
    // Initialize GPS
    GPS_SERIAL.begin(GPS_BAUD_RATE);
    delay(200);
    gpsInitialized = true;
    gpsEnabled = false;
    nmeaParserInit();
    
    // Configure analog inputs
    for (uint8_t i = 0; i < 16; ++i) {
        pinMode(A0 + i, INPUT_PULLUP);
    }
    
    // Configure digital inputs
    for (uint8_t pin = 22; pin <= 37; ++pin) {
        pinMode(pin, INPUT_PULLUP);
    }
    
    // Configure slow GPIO outputs
    for (uint8_t i = 0; i < 8; ++i) {
        pinMode(SLOW_GPIO_PINS[i], OUTPUT);
        digitalWrite(SLOW_GPIO_PINS[i], LOW);
    }
    
    // Configure PWM outputs
    for (uint8_t i = 0; i < 10; ++i) {
        pinMode(PWM_OUTPUT_PINS[i], OUTPUT);
        analogWrite(PWM_OUTPUT_PINS[i], 0);
    }
    
    // Disable I2C to free D20/D21
    TWCR &= ~(1<<TWEN);
    
    // Configure VSS pins
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
    
    // Attach VSS interrupts
    attachInterrupt(digitalPinToInterrupt(VSS_FRONT_LEFT_PIN), vssFrontLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_FRONT_RIGHT_PIN), vssFrontRightISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_REAR_LEFT_PIN), vssRearLeftISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(VSS_REAR_RIGHT_PIN), vssRearRightISR, FALLING);
    
    // Initialize TX states
    for (uint8_t i = 0; i < 16; ++i) {
        analogTxState[i].lastTransmittedValue = 0.0f;
        analogTxState[i].lastTxTime = 0;
        analogTxState[i].hasChanged = true;
        analogTxState[i].state = TX_STATE_CHANGED;
    }
    
    digitalTxState.lastTransmittedValue = 0.0f;
    digitalTxState.lastTxTime = 0;
    digitalTxState.hasChanged = true;
    digitalTxState.state = TX_STATE_CHANGED;
    
    for (uint8_t i = 0; i < 4; ++i) {
        vssTxState[i].lastTransmittedValue = 0.0f;
        vssTxState[i].lastTxTime = 0;
        vssTxState[i].hasChanged = true;
        vssTxState[i].state = TX_STATE_CHANGED;
    }
    
    for (uint8_t i = 0; i < 8; ++i) {
        gpsTxState[i].lastTransmittedValue = 0.0f;
        gpsTxState[i].lastTxTime = 0;
        gpsTxState[i].hasChanged = true;
        gpsTxState[i].state = TX_STATE_CHANGED;
    }
    
    Serial.println(F("Setup complete!"));
}

// ===== LOOP =====
void loop() {
    // Process CAN RX
    byte len = 0;
    byte buf[8];
    unsigned long canId;
    
    if(CAN.checkReceive() == CAN_MSGAVAIL) {
        CAN.readMsgBuf(&len, buf);
        canId = CAN.getCanId();
        
        if (canId == CAN_ID_VAR_RESPONSE && len == 8) {
            int32_t hash = readInt32BigEndian(&buf[0]);
            if (hash == VAR_HASH_OUT_SLOW) {
                float value = readFloat32BigEndian(&buf[4]);
                uint32_t rawBits = (value >= 0.0f) ? (uint32_t)(value + 0.5f) : 0u;
                
                // Slow GPIO
                uint8_t slowBits = (uint8_t)(rawBits & 0xFFu);
                for (uint8_t i = 0; i < 8; ++i) {
                    uint8_t pin = SLOW_GPIO_PINS[i];
                    if (slowBits & (1u << i)) {
                        digitalWrite(pin, HIGH);
                    } else {
                        digitalWrite(pin, LOW);
                    }
                }
                
                // PWM
                for (uint8_t i = 0; i < 10; ++i) {
                    uint8_t pin = PWM_OUTPUT_PINS[i];
                    uint8_t bitIndex = i + 8;
                    if (rawBits & (1u << bitIndex)) {
                        analogWrite(pin, 255);
                    } else {
                        analogWrite(pin, 0);
                    }
                }
            }
        }
    }
    
    // Calculate VSS
    calculateVSSRates();
    
    unsigned long nowMs = millis();
    
    // Request slow outputs
    static unsigned long lastSlowOutRequestMs = 0;
    if (nowMs - lastSlowOutRequestMs >= SLOW_OUT_REQUEST_INTERVAL_MS) {
        lastSlowOutRequestMs = nowMs;
        sendVariableRequestFrame(VAR_HASH_OUT_SLOW);
    }
    
    // Read GPS
    readGPSData();
    
    // Read inputs
    static unsigned long lastReadMs = 0;
    if (nowMs - lastReadMs >= TX_READ_INTERVAL_MS) {
        lastReadMs = nowMs;
        
        readAnalogInputs(currentAnalogValues);
        readDigitalInputs(&currentDigitalBits);
        readVSSInputs(currentVssValues);
        
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
    
    // Transmit
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
    
    transmitGPSIfNeeded();
}
