/*
  NMEA Parser Implementation
  Parses GPRMC and GPGGA sentences from GPS modules
*/

#include <Arduino.h>
#include "nmea_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>


// NMEA sentence buffer
char nmeaBuffer[NMEA_SENTENCE_MAX_LEN + 1];
uint8_t nmeaBufferIndex = 0;

// Initialize NMEA parser
void nmeaParserInit() {
    nmeaBufferIndex = 0;
    nmeaBuffer[0] = '\0';
}

// Parse time from NMEA format string (HHMMSS.xx, e.g., "230952.00" = 23:09:52)
static inline void parseTimeFromNMEAString(const char* timeStr, uint8_t* hours, uint8_t* minutes, uint8_t* seconds) {
    if (!timeStr || strlen(timeStr) < 6) {
        *hours = 0;
        *minutes = 0;
        *seconds = 0;
        return;
    }
    
    char hh[3] = {timeStr[0], timeStr[1], '\0'};
    char mm[3] = {timeStr[2], timeStr[3], '\0'};
    char ss[3] = {timeStr[4], timeStr[5], '\0'};
    
    *hours = (uint8_t)atoi(hh);
    *minutes = (uint8_t)atoi(mm);
    *seconds = (uint8_t)atoi(ss);
    
    if (*hours > 23) *hours = 0;
    if (*minutes > 59) *minutes = 0;
    if (*seconds > 59) *seconds = 0;
}

// Parse date from NMEA format string (DDMMYY, e.g., "201125" = 20 Nov, year 25)
// Returns 2-digit year (0-99) as it comes from NMEA
static inline void parseDateFromNMEAString(const char* dateStr, uint8_t* days, uint8_t* months, uint8_t* years) {
    if (!dateStr || strlen(dateStr) < 6) {
        *days = 1;
        *months = 1;
        *years = 25;  // Default to 25 (2025) if invalid
        return;
    }
    
    // Extract exactly 6 characters for date (DDMMYY)
    char dd[3] = {dateStr[0], dateStr[1], '\0'};
    char mm[3] = {dateStr[2], dateStr[3], '\0'};
    char yy[3] = {dateStr[4], dateStr[5], '\0'};
    
    *days = (uint8_t)atoi(dd);
    *months = (uint8_t)atoi(mm);
    *years = (uint8_t)atoi(yy);  // Keep as 2-digit year (0-99)
    
    if (*days < 1 || *days > 31) *days = 1;
    if (*months < 1 || *months > 12) *months = 1;
    if (*years > 99) *years = 25;  // Clamp to valid 2-digit year range
}

// Parse latitude from NMEA format (DDMM.MMMMMM, N/S)
// Returns decimal degrees (positive = North, negative = South)
static inline float parseLatitudeFromNMEA(const char* latStr, char ns) {
    if (!latStr || strlen(latStr) < 4) return 0.0f;
    
    // Extract degrees (first 2 digits)
    char degStr[3] = {latStr[0], latStr[1], '\0'};
    int degrees = atoi(degStr);
    
    // Extract minutes (everything from position 2 onwards, including decimal part)
    float minutes = atof(latStr + 2);
    
    float decimalDegrees = degrees + (minutes / 60.0f);
    if (ns == 'S' || ns == 's') {
        decimalDegrees = -decimalDegrees;
    }
    
    return decimalDegrees;
}

// Parse longitude from NMEA format (DDDMM.MMMMMM, E/W)
// Returns decimal degrees (positive = East, negative = West)
static inline float parseLongitudeFromNMEA(const char* lonStr, char ew) {
    if (!lonStr || strlen(lonStr) < 5) return 0.0f;
    
    // Extract degrees (first 3 digits)
    char degStr[4] = {lonStr[0], lonStr[1], lonStr[2], '\0'};
    int degrees = atoi(degStr);
    
    // Extract minutes (everything from position 3 onwards, including decimal part)
    float minutes = atof(lonStr + 3);
    
    float decimalDegrees = degrees + (minutes / 60.0f);
    if (ew == 'W' || ew == 'w') {
        decimalDegrees = -decimalDegrees;
    }
    
    return decimalDegrees;
}

// Get pointer to field in comma-separated NMEA sentence
// Returns NULL if field is empty or out of range
// Field 0 is the sentence type (e.g., "GPRMC"), field 1 is the first data field
const char* nmeaGetField(const char* sentence, uint8_t fieldIndex) {
    if (!sentence) return NULL;
    
    const char* p = sentence;
    uint8_t currentField = 0;
    
    // Skip $ if present
    if (*p == '$') p++;
    
    // Find the desired field
    const char* fieldStart = p;
    
    while (*p) {
        if (*p == ',' || *p == '*') {
            // End of current field
            if (currentField == fieldIndex) {
                // This is the field we want
                // Check if field is empty (fieldStart == p means empty field)
                if (fieldStart == p) {
                    return NULL;  // Empty field
                }
                return fieldStart;
            }
            // Move to next field
            currentField++;
            if (*p == ',') {
                p++;  // Skip comma
                fieldStart = p;  // Start of next field
            } else {
                // Hit * (checksum), no more fields
                break;
            }
        } else {
            p++;
        }
    }
    
    // Check if we're at the last field (no trailing comma)
    if (currentField == fieldIndex && fieldStart < p) {
        return fieldStart;
    }
    
    return NULL;  // Field index out of range
}

// Calculate NMEA checksum (XOR of all characters between $ and *)
static inline uint8_t calculateNMEAChecksum(const char* sentence) {
    if (!sentence || *sentence != '$') return 0;
    
    uint8_t checksum = 0;
    const char* p = sentence + 1;  // Skip $
    
    while (*p && *p != '*') {
        checksum ^= *p;
        p++;
    }
    
    return checksum;
}

// Verify NMEA sentence checksum
bool nmeaVerifyChecksum(const char* sentence) {
    if (!sentence) return false;
    
    const char* star = strchr(sentence, '*');
    if (!star || strlen(star) < 3) return false;
    
    uint8_t calculated = calculateNMEAChecksum(sentence);
    uint8_t received = (uint8_t)strtoul(star + 1, NULL, 16);
    
    return (calculated == received);
}

// Parse GPRMC sentence (Recommended Minimum Course)
// Format: $GPRMC,time,status,lat,N/S,lon,E/W,speed,course,date,mag_var,E/W,mode*checksum
static bool parseGPRMC(const char* sentence, struct GPSData* gpsData) {
    if (!sentence || strncmp(sentence, "$GPRMC", 6) != 0) return false;
    if (!nmeaVerifyChecksum(sentence)) return false;
    
    const char* timeStr = nmeaGetField(sentence, 1);  // Field 1: time
    const char* statusStr = nmeaGetField(sentence, 2);  // Field 2: status (A=valid, V=invalid)
    const char* latStr = nmeaGetField(sentence, 3);  // Field 3: latitude
    const char* nsStr = nmeaGetField(sentence, 4);  // Field 4: N/S
    const char* lonStr = nmeaGetField(sentence, 5);  // Field 5: longitude
    const char* ewStr = nmeaGetField(sentence, 6);  // Field 6: E/W
    const char* speedStr = nmeaGetField(sentence, 7);  // Field 7: speed (knots)
    const char* courseStr = nmeaGetField(sentence, 8);  // Field 8: course
    const char* dateStr = nmeaGetField(sentence, 9);  // Field 9: date
    
    // DEBUG: Print extracted fields (extract up to comma for display)
    Serial.print("[GPRMC] Fields - time: ");
    if (timeStr) {
        const char* end = strchr(timeStr, ',');
        if (end) {
            for (const char* p = timeStr; p < end && p < timeStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(timeStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", status: ");
    if (statusStr) {
        const char* end = strchr(statusStr, ',');
        if (end) {
            for (const char* p = statusStr; p < end && p < statusStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(statusStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", date: ");
    if (dateStr) {
        const char* end = strchr(dateStr, ',');
        if (end) {
            for (const char* p = dateStr; p < end && p < dateStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(dateStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", lat: ");
    if (latStr) {
        const char* end = strchr(latStr, ',');
        if (end) {
            for (const char* p = latStr; p < end && p < latStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(latStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", lon: ");
    if (lonStr) {
        const char* end = strchr(lonStr, ',');
        if (end) {
            for (const char* p = lonStr; p < end && p < lonStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(lonStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.println();
    
    if (!timeStr || !statusStr || !dateStr) {
        Serial.println("[GPRMC] Missing required fields!");
        return false;
    }
    
    // Parse time - ensure we only read up to comma or end
    char timeBuf[16] = {0};
    const char* timeEnd = strchr(timeStr, ',');
    if (timeEnd) {
        size_t len = timeEnd - timeStr;
        if (len > 15) len = 15;
        strncpy(timeBuf, timeStr, len);
        timeBuf[len] = '\0';
    } else {
        strncpy(timeBuf, timeStr, 15);
        timeBuf[15] = '\0';
    }
    parseTimeFromNMEAString(timeBuf, &gpsData->hours, &gpsData->minutes, &gpsData->seconds);
    
    // Parse date - ensure we only read exactly 6 characters
    char dateBuf[7] = {0};
    const char* dateEnd = strchr(dateStr, ',');
    if (dateEnd) {
        size_t len = dateEnd - dateStr;
        if (len > 6) len = 6;
        strncpy(dateBuf, dateStr, len);
        dateBuf[len] = '\0';
    } else {
        strncpy(dateBuf, dateStr, 6);
        dateBuf[6] = '\0';
    }
    parseDateFromNMEAString(dateBuf, &gpsData->days, &gpsData->months, &gpsData->years);
    
    // Check if fix is valid
    bool hasValidFix = (*statusStr == 'A' || *statusStr == 'a');
    gpsData->hasFix = hasValidFix;
    
    // Parse position if fix is valid
    if (hasValidFix && latStr && nsStr && lonStr && ewStr) {
        // Extract latitude field (stop at comma)
        char latBuf[16] = {0};
        const char* latEnd = strchr(latStr, ',');
        if (latEnd) {
            size_t len = latEnd - latStr;
            if (len > 15) len = 15;
            strncpy(latBuf, latStr, len);
            latBuf[len] = '\0';
        } else {
            strncpy(latBuf, latStr, 15);
            latBuf[15] = '\0';
        }
        gpsData->latitude = parseLatitudeFromNMEA(latBuf, *nsStr);
        
        // Extract longitude field (stop at comma)
        char lonBuf[16] = {0};
        const char* lonEnd = strchr(lonStr, ',');
        if (lonEnd) {
            size_t len = lonEnd - lonStr;
            if (len > 15) len = 15;
            strncpy(lonBuf, lonStr, len);
            lonBuf[len] = '\0';
        } else {
            strncpy(lonBuf, lonStr, 15);
            lonBuf[15] = '\0';
        }
        gpsData->longitude = parseLongitudeFromNMEA(lonBuf, *ewStr);
        
        // Parse speed (knots to km/h)
        if (speedStr) {
            char speedBuf[16] = {0};
            const char* speedEnd = strchr(speedStr, ',');
            if (speedEnd) {
                size_t len = speedEnd - speedStr;
                if (len > 15) len = 15;
                strncpy(speedBuf, speedStr, len);
                speedBuf[len] = '\0';
            } else {
                strncpy(speedBuf, speedStr, 15);
                speedBuf[15] = '\0';
            }
            float speedKnots = atof(speedBuf);
            gpsData->speed = speedKnots * 1.852f;  // Convert knots to km/h
        }
        
        // Parse course
        if (courseStr) {
            char courseBuf[16] = {0};
            const char* courseEnd = strchr(courseStr, ',');
            if (courseEnd) {
                size_t len = courseEnd - courseStr;
                if (len > 15) len = 15;
                strncpy(courseBuf, courseStr, len);
                courseBuf[len] = '\0';
            } else {
                strncpy(courseBuf, courseStr, 15);
                courseBuf[15] = '\0';
            }
            gpsData->course = atof(courseBuf);
        }
    }
    
    gpsData->dataValid = true;
    return true;
}

// Parse GPGGA sentence (Global Positioning System Fix Data)
// Format: $GPGGA,time,lat,N/S,lon,E/W,quality,num_sats,hdop,altitude,M,sep,M,diff_age,diff_station*checksum
static bool parseGPGGA(const char* sentence, struct GPSData* gpsData) {
    if (!sentence || strncmp(sentence, "$GPGGA", 6) != 0) return false;
    if (!nmeaVerifyChecksum(sentence)) return false;
    
    const char* latStr = nmeaGetField(sentence, 2);  // Field 2: latitude
    const char* nsStr = nmeaGetField(sentence, 3);  // Field 3: N/S
    const char* lonStr = nmeaGetField(sentence, 4);  // Field 4: longitude
    const char* ewStr = nmeaGetField(sentence, 5);  // Field 5: E/W
    const char* qualityStr = nmeaGetField(sentence, 6);  // Field 6: quality (0=no fix, 1=GPS, 2=DGPS)
    const char* numSatsStr = nmeaGetField(sentence, 7);  // Field 7: number of satellites
    const char* hdopStr = nmeaGetField(sentence, 8);  // Field 8: HDOP (accuracy)
    const char* altitudeStr = nmeaGetField(sentence, 9);  // Field 9: altitude
    
    // DEBUG: Print extracted fields (extract up to comma for display)
    Serial.print("[GPGGA] Fields - quality: ");
    if (qualityStr) {
        const char* end = strchr(qualityStr, ',');
        if (end) {
            for (const char* p = qualityStr; p < end && p < qualityStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(qualityStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", sats: ");
    if (numSatsStr) {
        const char* end = strchr(numSatsStr, ',');
        if (end) {
            for (const char* p = numSatsStr; p < end && p < numSatsStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(numSatsStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", lat: ");
    if (latStr) {
        const char* end = strchr(latStr, ',');
        if (end) {
            for (const char* p = latStr; p < end && p < latStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(latStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.print(", lon: ");
    if (lonStr) {
        const char* end = strchr(lonStr, ',');
        if (end) {
            for (const char* p = lonStr; p < end && p < lonStr + 20; p++) Serial.print(*p);
        } else {
            Serial.print(lonStr);
        }
    } else {
        Serial.print("(NULL)");
    }
    Serial.println();
    
    if (!qualityStr) {
        Serial.println("[GPGGA] Missing quality field!");
        return false;
    }
    
    // Extract quality field (stop at comma)
    char qualityBuf[4] = {0};
    const char* qualityEnd = strchr(qualityStr, ',');
    if (qualityEnd) {
        size_t len = qualityEnd - qualityStr;
        if (len > 3) len = 3;
        strncpy(qualityBuf, qualityStr, len);
        qualityBuf[len] = '\0';
    } else {
        strncpy(qualityBuf, qualityStr, 3);
        qualityBuf[3] = '\0';
    }
    gpsData->quality = (uint8_t)atoi(qualityBuf);
    
    // Parse satellites
    if (numSatsStr) {
        char satsBuf[4] = {0};
        const char* satsEnd = strchr(numSatsStr, ',');
        if (satsEnd) {
            size_t len = satsEnd - numSatsStr;
            if (len > 3) len = 3;
            strncpy(satsBuf, numSatsStr, len);
            satsBuf[len] = '\0';
        } else {
            strncpy(satsBuf, numSatsStr, 3);
            satsBuf[3] = '\0';
        }
        gpsData->satellites = (uint8_t)atoi(satsBuf);
    }
    
    // Quality > 0 means we have a fix
    if (gpsData->quality > 0) {
        gpsData->hasFix = true;
        
        // Parse position if available
        if (latStr && nsStr && lonStr && ewStr) {
            // Extract latitude field (stop at comma)
            char latBuf[16] = {0};
            const char* latEnd = strchr(latStr, ',');
            if (latEnd) {
                size_t len = latEnd - latStr;
                if (len > 15) len = 15;
                strncpy(latBuf, latStr, len);
                latBuf[len] = '\0';
            } else {
                strncpy(latBuf, latStr, 15);
                latBuf[15] = '\0';
            }
            gpsData->latitude = parseLatitudeFromNMEA(latBuf, *nsStr);
            
            // Extract longitude field (stop at comma)
            char lonBuf[16] = {0};
            const char* lonEnd = strchr(lonStr, ',');
            if (lonEnd) {
                size_t len = lonEnd - lonStr;
                if (len > 15) len = 15;
                strncpy(lonBuf, lonStr, len);
                lonBuf[len] = '\0';
            } else {
                strncpy(lonBuf, lonStr, 15);
                lonBuf[15] = '\0';
            }
            gpsData->longitude = parseLongitudeFromNMEA(lonBuf, *ewStr);
        }
        
        // Parse altitude
        if (altitudeStr) {
            char altBuf[16] = {0};
            const char* altEnd = strchr(altitudeStr, ',');
            if (altEnd) {
                size_t len = altEnd - altitudeStr;
                if (len > 15) len = 15;
                strncpy(altBuf, altitudeStr, len);
                altBuf[len] = '\0';
            } else {
                strncpy(altBuf, altitudeStr, 15);
                altBuf[15] = '\0';
            }
            gpsData->altitude = atof(altBuf);
        }
        
        // Parse HDOP (accuracy) - lower is better
        if (hdopStr) {
            char hdopBuf[16] = {0};
            const char* hdopEnd = strchr(hdopStr, ',');
            if (hdopEnd) {
                size_t len = hdopEnd - hdopStr;
                if (len > 15) len = 15;
                strncpy(hdopBuf, hdopStr, len);
                hdopBuf[len] = '\0';
            } else {
                strncpy(hdopBuf, hdopStr, 15);
                hdopBuf[15] = '\0';
            }
            gpsData->accuracy = atof(hdopBuf);
        }
    } else {
        gpsData->hasFix = false;
    }
    
    gpsData->dataValid = true;
    return true;
}

// Process incoming character, returns true if a complete sentence was parsed
bool nmeaParserProcessChar(char c, struct GPSData* gpsData) {
    // Start of sentence
    if (c == '$') {
        nmeaBufferIndex = 0;
        nmeaBuffer[0] = c;
        nmeaBufferIndex = 1;
        return false;
    }
    // End of sentence (CR or LF)
    else if (c == '\r' || c == '\n') {
        if (nmeaBufferIndex > 0) {
            nmeaBuffer[nmeaBufferIndex] = '\0';  // Null terminate
            
            // DEBUG: Print raw NMEA sentence
            Serial.print("[NMEA] Raw: ");
            Serial.println(nmeaBuffer);
            
            // Parse the sentence
            bool parsed = false;
            if (strncmp(nmeaBuffer, "$GPRMC", 6) == 0) {
                Serial.println("[NMEA] Parsing GPRMC...");
                parsed = parseGPRMC(nmeaBuffer, gpsData);
                if (parsed) {
                    Serial.print("[NMEA] GPRMC parsed OK - Time: ");
                    Serial.print(gpsData->hours);
                    Serial.print(":");
                    Serial.print(gpsData->minutes);
                    Serial.print(":");
                    Serial.print(gpsData->seconds);
                    Serial.print(", Date: ");
                    Serial.print(gpsData->days);
                    Serial.print("/");
                    Serial.print(gpsData->months);
                    Serial.print("/");
                    if (gpsData->years < 10) Serial.print("0");
                    Serial.println(gpsData->years);
                } else {
                    Serial.println("[NMEA] GPRMC parse FAILED");
                }
            } else if (strncmp(nmeaBuffer, "$GPGGA", 6) == 0) {
                Serial.println("[NMEA] Parsing GPGGA...");
                parsed = parseGPGGA(nmeaBuffer, gpsData);
                if (parsed) {
                    Serial.print("[NMEA] GPGGA parsed OK - Quality: ");
                    Serial.print(gpsData->quality);
                    Serial.print(", Sats: ");
                    Serial.print(gpsData->satellites);
                    Serial.print(", Lat: ");
                    Serial.print(gpsData->latitude, 6);
                    Serial.print(", Lon: ");
                    Serial.println(gpsData->longitude, 6);
                } else {
                    Serial.println("[NMEA] GPGGA parse FAILED");
                }
            } else {
                // Other sentence types - just print for reference
                Serial.print("[NMEA] Ignored sentence type: ");
                if (nmeaBufferIndex >= 6) {
                    char type[7] = {0};
                    strncpy(type, nmeaBuffer, 6);
                    Serial.println(type);
                } else {
                    Serial.println("(too short)");
                }
            }
            
            // Reset buffer
            nmeaBufferIndex = 0;
            return parsed;
        }
        nmeaBufferIndex = 0;
        return false;
    }
    // Add character to buffer (if we have space)
    else if (nmeaBufferIndex < NMEA_SENTENCE_MAX_LEN) {
        nmeaBuffer[nmeaBufferIndex++] = c;
        return false;
    }
    // Buffer overflow - reset
    else {
        nmeaBufferIndex = 0;
        return false;
    }
}

