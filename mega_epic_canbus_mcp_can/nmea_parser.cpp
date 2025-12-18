/*
  nmea_parser.cpp - Lightweight NMEA 0183 GPS Parser Implementation
*/

#include "nmea_parser.h"
#include <string.h>
#include <stdlib.h>

// Parser state
static char nmeaBuffer[100];
static uint8_t nmeaIndex = 0;
static bool inSentence = false;

// Helper: Parse NMEA time field (HHMMSS.sss)
static void parseTime(const char* field, GPSData* data) {
    if (!field || strlen(field) < 6) return;
    
    char buf[3];
    buf[2] = '\0';
    
    buf[0] = field[0]; buf[1] = field[1];
    data->hours = atoi(buf);
    
    buf[0] = field[2]; buf[1] = field[3];
    data->minutes = atoi(buf);
    
    buf[0] = field[4]; buf[1] = field[5];
    data->seconds = atoi(buf);
}

// Helper: Parse NMEA date field (DDMMYY)
static void parseDate(const char* field, GPSData* data) {
    if (!field || strlen(field) < 6) return;
    
    char buf[3];
    buf[2] = '\0';
    
    buf[0] = field[0]; buf[1] = field[1];
    data->days = atoi(buf);
    
    buf[0] = field[2]; buf[1] = field[3];
    data->months = atoi(buf);
    
    buf[0] = field[4]; buf[1] = field[5];
    data->years = atoi(buf);
}

// Helper: Parse latitude (DDMM.MMMM format) to decimal degrees
static float parseLatitude(const char* field, char direction) {
    if (!field || strlen(field) < 4) return 0.0;
    
    // Extract degrees (first 2 chars)
    char degStr[3] = {field[0], field[1], '\0'};
    float degrees = atof(degStr);
    
    // Extract minutes (rest of string)
    float minutes = atof(field + 2);
    
    // Convert to decimal degrees
    float decimal = degrees + (minutes / 60.0);
    
    // Apply direction
    if (direction == 'S' || direction == 'W') {
        decimal = -decimal;
    }
    
    return decimal;
}

// Helper: Parse longitude (DDDMM.MMMM format) to decimal degrees  
static float parseLongitude(const char* field, char direction) {
    if (!field || strlen(field) < 5) return 0.0;
    
    // Extract degrees (first 3 chars)
    char degStr[4] = {field[0], field[1], field[2], '\0'};
    float degrees = atof(degStr);
    
    // Extract minutes (rest of string)
    float minutes = atof(field + 3);
    
    // Convert to decimal degrees
    float decimal = degrees + (minutes / 60.0);
    
    // Apply direction
    if (direction == 'S' || direction == 'W') {
        decimal = -decimal;
    }
    
    return decimal;
}

// Helper: Calculate NMEA checksum
static uint8_t calculateChecksum(const char* sentence) {
    uint8_t checksum = 0;
    const char* p = sentence + 1;  // Skip $
    
    while (*p && *p != '*') {
        checksum ^= *p;
        p++;
    }
    
    return checksum;
}

// Helper: Verify NMEA checksum
static bool verifyChecksum(const char* sentence) {
    const char* asterisk = strchr(sentence, '*');
    if (!asterisk || asterisk - sentence < 2) return false;
    
    uint8_t calculated = calculateChecksum(sentence);
    uint8_t provided = (uint8_t)strtol(asterisk + 1, NULL, 16);
    
    return (calculated == provided);
}

// Parse GPRMC sentence: $GPRMC,time,status,lat,N/S,lon,E/W,speed,course,date,,,*checksum
static bool parseGPRMC(const char* sentence, GPSData* data) {
    char localCopy[100];
    strncpy(localCopy, sentence, sizeof(localCopy) - 1);
    localCopy[sizeof(localCopy) - 1] = '\0';
    
    char* fields[15];
    uint8_t fieldCount = 0;
    
    char* token = strtok(localCopy, ",");
    while (token && fieldCount < 15) {
        fields[fieldCount++] = token;
        token = strtok(NULL, ",");
    }
    
    if (fieldCount < 9) return false;
    
    // Field 0: $GPRMC
    // Field 1: Time
    if (strlen(fields[1]) >= 6) {
        parseTime(fields[1], data);
    }
    
    // Field 2: Status (A=active, V=void)
    data->dataValid = (fields[2][0] == 'A');
    
    // Field 3-4: Latitude
    if (strlen(fields[3]) > 0 && strlen(fields[4]) > 0) {
        data->latitude = parseLatitude(fields[3], fields[4][0]);
    }
    
    // Field 5-6: Longitude
    if (strlen(fields[5]) > 0 && strlen(fields[6]) > 0) {
        data->longitude = parseLongitude(fields[5], fields[6][0]);
    }
    
    // Field 7: Speed in knots
    if (strlen(fields[7]) > 0) {
        data->speed = atof(fields[7]);
    }
    
    // Field 8: Course
    if (strlen(fields[8]) > 0) {
        data->course = atof(fields[8]);
    }
    
    // Field 9: Date
    if (fieldCount > 9 && strlen(fields[9]) >= 6) {
        parseDate(fields[9], data);
    }
    
    data->hasFix = data->dataValid;
    
    return true;
}

// Parse GPGGA sentence: $GPGGA,time,lat,N/S,lon,E/W,quality,sats,hdop,alt,M,...*checksum
static bool parseGPGGA(const char* sentence, GPSData* data) {
    char localCopy[100];
    strncpy(localCopy, sentence, sizeof(localCopy) - 1);
    localCopy[sizeof(localCopy) - 1] = '\0';
    
    char* fields[15];
    uint8_t fieldCount = 0;
    
    char* token = strtok(localCopy, ",");
    while (token && fieldCount < 15) {
        fields[fieldCount++] = token;
        token = strtok(NULL, ",");
    }
    
    if (fieldCount < 10) return false;
    
    // Field 0: $GPGGA
    // Field 1: Time
    if (strlen(fields[1]) >= 6) {
        parseTime(fields[1], data);
    }
    
    // Field 2-3: Latitude
    if (strlen(fields[2]) > 0 && strlen(fields[3]) > 0) {
        data->latitude = parseLatitude(fields[2], fields[3][0]);
    }
    
    // Field 4-5: Longitude
    if (strlen(fields[4]) > 0 && strlen(fields[5]) > 0) {
        data->longitude = parseLongitude(fields[4], fields[5][0]);
    }
    
    // Field 6: Fix quality
    if (strlen(fields[6]) > 0) {
        data->quality = atoi(fields[6]);
        data->hasFix = (data->quality > 0);
    }
    
    // Field 7: Number of satellites
    if (strlen(fields[7]) > 0) {
        data->satellites = atoi(fields[7]);
    }
    
    // Field 8: HDOP
    if (strlen(fields[8]) > 0) {
        data->accuracy = atof(fields[8]);
    }
    
    // Field 9: Altitude
    if (strlen(fields[9]) > 0) {
        data->altitude = atof(fields[9]);
    }
    
    return true;
}

// Initialize parser
void nmeaParserInit() {
    nmeaIndex = 0;
    inSentence = false;
    memset(nmeaBuffer, 0, sizeof(nmeaBuffer));
}

// Process single character
bool nmeaParserProcessChar(char c, GPSData* data) {
    // Start of sentence
    if (c == '$') {
        nmeaIndex = 0;
        inSentence = true;
        nmeaBuffer[nmeaIndex++] = c;
        return false;
    }
    
    // Not in a sentence
    if (!inSentence) {
        return false;
    }
    
    // End of sentence
    if (c == '\r' || c == '\n') {
        if (nmeaIndex > 0) {
            nmeaBuffer[nmeaIndex] = '\0';
            inSentence = false;
            
            // Verify checksum
            if (!verifyChecksum(nmeaBuffer)) {
                nmeaIndex = 0;
                return false;
            }
            
            // Parse sentence type
            bool parsed = false;
            if (strncmp(nmeaBuffer, "$GPRMC", 6) == 0 || 
                strncmp(nmeaBuffer, "$GNRMC", 6) == 0) {
                parsed = parseGPRMC(nmeaBuffer, data);
            } else if (strncmp(nmeaBuffer, "$GPGGA", 6) == 0 || 
                       strncmp(nmeaBuffer, "$GNGGA", 6) == 0) {
                parsed = parseGPGGA(nmeaBuffer, data);
            }
            
            nmeaIndex = 0;
            return parsed;
        }
        return false;
    }
    
    // Add character to buffer
    if (nmeaIndex < sizeof(nmeaBuffer) - 1) {
        nmeaBuffer[nmeaIndex++] = c;
    } else {
        // Buffer overflow - reset
        nmeaIndex = 0;
        inSentence = false;
    }
    
    return false;
}
