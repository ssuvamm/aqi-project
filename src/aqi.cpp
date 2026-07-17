#include "aqi.h"
#include <TFT_eSPI.h>  // for TFT_* color constants

namespace {
struct Breakpoint { float cLo, cHi; int iLo, iHi; };

// PM2.5 (ug/m3, 24 h) — EPA May 2024 revision
const Breakpoint PM25_BP[] = {
    {  0.0f,   9.0f,   0,  50},
    {  9.1f,  35.4f,  51, 100},
    { 35.5f,  55.4f, 101, 150},
    { 55.5f, 125.4f, 151, 200},
    {125.5f, 225.4f, 201, 300},
    {225.5f, 325.4f, 301, 500},
};

// PM10 (ug/m3, 24 h)
const Breakpoint PM10_BP[] = {
    {  0.0f,  54.0f,   0,  50},
    { 55.0f, 154.0f,  51, 100},
    {155.0f, 254.0f, 101, 150},
    {255.0f, 354.0f, 151, 200},
    {355.0f, 424.0f, 201, 300},
    {425.0f, 604.0f, 301, 500},
};

int interpolate(const Breakpoint* table, size_t n, float c) {
    if (c < 0) return -1;
    for (size_t i = 0; i < n; i++) {
        const Breakpoint& b = table[i];
        if (c <= b.cHi) {
            float aqi = (float)(b.iHi - b.iLo) / (b.cHi - b.cLo) * (c - b.cLo) + b.iLo;
            return (int)(aqi + 0.5f);
        }
    }
    return 500;  // beyond the last breakpoint
}
}

int aqiFromPm25(float ugm3) { return interpolate(PM25_BP, 6, ugm3); }
int aqiFromPm10(float ugm3) { return interpolate(PM10_BP, 6, ugm3); }

AqiCategory aqiCategory(int aqi) {
    if (aqi <= 50)  return AQI_GOOD;
    if (aqi <= 100) return AQI_MODERATE;
    if (aqi <= 150) return AQI_SENSITIVE;
    if (aqi <= 200) return AQI_UNHEALTHY;
    if (aqi <= 300) return AQI_VERY_UNHEALTHY;
    return AQI_HAZARDOUS;
}

const char* aqiCategoryName(AqiCategory c) {
    switch (c) {
        case AQI_GOOD:           return "GOOD";
        case AQI_MODERATE:       return "MODERATE";
        case AQI_SENSITIVE:      return "SENSITIVE";
        case AQI_UNHEALTHY:      return "UNHEALTHY";
        case AQI_VERY_UNHEALTHY: return "V.UNHEALTHY";
        default:                 return "HAZARDOUS";
    }
}

uint16_t aqiCategoryColor(AqiCategory c) {
    switch (c) {
        case AQI_GOOD:           return TFT_GREEN;
        case AQI_MODERATE:       return TFT_YELLOW;
        case AQI_SENSITIVE:      return TFT_ORANGE;
        case AQI_UNHEALTHY:      return TFT_RED;
        case AQI_VERY_UNHEALTHY: return TFT_PURPLE;
        default:                 return TFT_MAROON;
    }
}

const char* co2Label(int ppm) {
    if (ppm < 800)  return "FRESH";
    if (ppm < 1200) return "OK";
    if (ppm < 2000) return "STUFFY";
    return "BAD";
}

uint16_t co2Color(int ppm) {
    if (ppm < 800)  return TFT_GREEN;
    if (ppm < 1200) return TFT_YELLOW;
    if (ppm < 2000) return TFT_ORANGE;
    return TFT_RED;
}
