#pragma once
#include <Arduino.h>

// US EPA AQI computation using the May-2024 revised PM2.5 breakpoints.
// Note: EPA breakpoints are defined for 24-hour averages; applying them to
// instantaneous readings is a "NowCast-style" approximation that every
// consumer monitor uses. Values match uRADMonitor/AirNow behaviour closely.

enum AqiCategory : uint8_t {
    AQI_GOOD = 0,
    AQI_MODERATE,
    AQI_SENSITIVE,      // Unhealthy for Sensitive Groups
    AQI_UNHEALTHY,
    AQI_VERY_UNHEALTHY,
    AQI_HAZARDOUS,
};

int aqiFromPm25(float ugm3);   // -1 if input invalid
int aqiFromPm10(float ugm3);

AqiCategory aqiCategory(int aqi);
const char* aqiCategoryName(AqiCategory c);   // short, fits the display
uint16_t aqiCategoryColor(AqiCategory c);     // RGB565

// CO2 comfort bands (indoor-air guidance, not an EPA scale)
const char* co2Label(int ppm);
uint16_t co2Color(int ppm);
