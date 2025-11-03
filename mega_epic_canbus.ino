/*  receive a frame from can bus

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

// Please modify SPI_CS_PIN to adapt to your board (see table below)
#define SPI_CS_PIN  9 

MCP_CAN CAN(SPI_CS_PIN);                                    // Set CS pin

// ECU and EPIC CAN configuration
#define ECU_CAN_ID  1
#define TRANSMIT_INTERVAL_MS 25
#define DIGITAL_OUTPUT_POLL_INTERVAL_MS 50  // 20 Hz polling for digital outputs
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

static inline void sendVariableSetFrame(int32_t varHash, float value)
{
    unsigned char data[8];
    writeInt32BigEndian(varHash, &data[0]);
    writeFloat32BigEndian(value, &data[4]);
    CAN.sendMsgBuf(CAN_ID_VARIABLE_SET, 0, 8, data);
}

static inline bool sendVariableRequest(int32_t varHash)
{
    unsigned char data[4];
    writeInt32BigEndian(varHash, &data[0]);
    byte ret = CAN.sendMsgBuf(CAN_ID_VAR_REQUEST, 0, 4, data);
    return (ret == CAN_SENDMSGTIMEOUT || ret == CAN_OK);
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
    
    Serial.println("Pin initialization complete");
    
    // Check if digital output hash is configured
    if (VAR_HASH_D35_D49 == 0)
    {
        Serial.println("WARNING: VAR_HASH_D35_D49 not set - digital outputs disabled");
    }
}


// Process incoming CAN frames (EPIC protocol)
static void processCanFrame(unsigned long canId, unsigned char len, unsigned char* buf)
{
    // Validate frame length (EPIC frames should be 8 bytes, except var_request which is 4)
    if (len != 8 && len != 4)
    {
        Serial.print("Invalid frame length: ");
        Serial.println(len);
        return;
    }
    
    // Parse variable_response frames (0x720 + ecuCanId)
    if (IS_VAR_RESPONSE(canId))
    {
        if (len != 8)
        {
            Serial.println("Error: Variable response must be 8 bytes");
            return;
        }
        
        // Extract variable hash (bytes 0-3)
        int32_t varHash = readInt32BigEndian(&buf[0]);
        
        // Extract variable value (bytes 4-7)
        float value = readFloat32BigEndian(&buf[4]);
        
        // Handle digital output bitfield (D35-D49)
        if (VAR_HASH_D35_D49 != 0 && varHash == VAR_HASH_D35_D49)
        {
            // Unpack 15-bit bitfield and apply to D35-D49
            uint16_t bits = (uint16_t)value;  // Cast float to uint16_t (bitfield)
            
            for (uint8_t pin = 35; pin <= 49; ++pin)
            {
                uint8_t bitIndex = (uint8_t)(pin - 35);
                bool state = (bits >> bitIndex) & 1;
                digitalWrite(pin, state ? HIGH : LOW);
            }
            
            // Optional debug output (comment out for production)
            // Serial.print("Digital outputs updated: 0x");
            // Serial.println(bits, HEX);
        }
        else
        {
            // Other variable responses can be handled here (e.g., PWM values)
            // Serial.print("Var response - Hash: ");
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
            return;
        }
        
        // Extract function ID (bytes 0-1)
        uint16_t funcId = ((uint16_t)buf[0] << 8) | buf[1];
        
        // Extract return value (bytes 2-5 as float32)
        float returnValue = readFloat32BigEndian(&buf[2]);
        
        // Function response handling can be implemented here
        // Serial.print("Function response - ID: ");
        // Serial.print(funcId);
        // Serial.print(", Return: ");
        // Serial.println(returnValue);
    }
    // Parse variable_set frames (if ECU sends variable_set to Mega - optional)
    else if (IS_VAR_SET(canId))
    {
        if (len != 8)
        {
            Serial.println("Error: Variable set must be 8 bytes");
            return;
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

    // Periodic transmission of analog and digital inputs
    static unsigned long lastTxMs = 0;
    static unsigned long lastDigitalOutputRequestMs = 0;
    unsigned long nowMs = millis();
    
    if (nowMs - lastTxMs >= TRANSMIT_INTERVAL_MS)
    {
        lastTxMs = nowMs;

        // Transmit analog inputs A0..A15 (raw ADC counts as float32)
        for (uint8_t i = 0; i < 16; ++i)
        {
            int adc = analogRead(A0 + i);
            float value = (float)adc;
            sendVariableSetFrame(VAR_HASH_ANALOG[i], value);
            // Serial.print("Analog ");
            // Serial.print(i);
            // Serial.print(": ");
            // Serial.println(value);
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
        sendVariableSetFrame(VAR_HASH_D20_D34, (float)bits);
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
}

// END FILE
