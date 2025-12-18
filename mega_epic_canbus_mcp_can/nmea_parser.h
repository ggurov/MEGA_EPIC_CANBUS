/*
  nmea_parser.h - Lightweight NMEA 0183 GPS Parser
  Parses GPRMC and GPGGA sentences for basic GPS data
*/

#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <Arduino.h>

// GPS data structure
struct GPSData {
    // Time and date
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t days;
    uint8_t months;
    uint8_t years;  // Years since 2000 (e.g., 24 for 2024)
    
    // Position
    float latitude;   // Decimal degrees
    float longitude;  // Decimal degrees
    float altitude;   // Meters above sea level
    
    // Motion
    float speed;      // Speed in knots
    float course;     // Course over ground in degrees
    
    // Quality
    uint8_t quality;     // Fix quality (0=invalid, 1=GPS, 2=DGPS)
    uint8_t satellites;  // Number of satellites in use
    float accuracy;      // HDOP (horizontal dilution of precision)
    
    // Status
    bool dataValid;   // True if GPS has valid data
    bool hasFix;      // True if GPS has a position fix
};

// Initialize NMEA parser
void nmeaParserInit();

// Process a single character from GPS
// Returns true if a complete valid sentence was parsed
bool nmeaParserProcessChar(char c, GPSData* data);

#endif // NMEA_PARSER_H
