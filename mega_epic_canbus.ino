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
#define TRANSMIT_INTERVAL_MS 25
#define CAN_ID_VAR_REQUEST        (0x700 + ECU_CAN_ID)
#define CAN_ID_VAR_RESPONSE       (0x720 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_REQUEST   (0x740 + ECU_CAN_ID)
#define CAN_ID_FUNCTION_RESPONSE  (0x760 + ECU_CAN_ID)
#define CAN_ID_VARIABLE_SET       (0x780 + ECU_CAN_ID)

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

static inline void sendVariableSetFrame(int32_t varHash, float value)
{
    unsigned char data[8];
    writeInt32BigEndian(varHash, &data[0]);
    writeFloat32BigEndian(value, &data[4]);
    CAN.sendMsgBuf(CAN_ID_VARIABLE_SET, 0, 8, data);
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

    // Periodic transmission of analog and digital inputs
    static unsigned long lastTxMs = 0;
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
}

// END FILE
