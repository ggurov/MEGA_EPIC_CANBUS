/*
  NMEA Parser for GPS modules
  Parses GPRMC and GPGGA sentences
*/

#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <stdint.h>
#include <stdbool.h>

// GPS data structure
struct GPSData {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t days;
    uint8_t months;
    uint8_t years;  // 2-digit year from NMEA (0-99, e.g., 25 for 2025)
    uint8_t quality;
    uint8_t satellites;
    float accuracy;
    float altitude;
    float course;
    float latitude;
    float longitude;
    float speed;
    bool hasFix;
    bool dataValid;  // True if GPS has provided valid data at least once
};

// NMEA parser state
#define NMEA_SENTENCE_MAX_LEN 82
extern char nmeaBuffer[NMEA_SENTENCE_MAX_LEN + 1];
extern uint8_t nmeaBufferIndex;

// Initialize NMEA parser
void nmeaParserInit();

// Process incoming character, returns true if a complete sentence was parsed
bool nmeaParserProcessChar(char c, struct GPSData* gpsData);

// Get pointer to field in comma-separated NMEA sentence
// Returns NULL if field is empty or out of range
const char* nmeaGetField(const char* sentence, uint8_t fieldIndex);

// Verify NMEA sentence checksum
bool nmeaVerifyChecksum(const char* sentence);

#endif // NMEA_PARSER_H

