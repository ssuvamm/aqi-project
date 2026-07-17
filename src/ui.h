#pragma once
#include <Arduino.h>

// Everything the display needs for one refresh. main.cpp fills this once per
// sensor cycle; the UI module owns the TFT and decides how to render it.
struct UiModel {
    bool pmValid = false;      // at least one good frame, not stale
    bool co2Valid = false;
    bool pmWarmup = true;      // inside datasheet warm-up window
    bool co2Warmup = true;

    uint16_t pm1 = 0, pm25 = 0, pm10 = 0;          // atmospheric, ug/m3
    uint16_t n03 = 0, n05 = 0, n10 = 0;            // particles per 0.1 L
    uint16_t n25 = 0, n50 = 0, n100 = 0;
    int co2 = -1;              // ppm
    int temp = 0;              // MH-Z19E internal temp, rough
    int aqi = -1;              // combined US AQI (max of PM2.5/PM10 sub-indices)
    int aqiPm25 = -1, aqiPm10 = -1;
    uint32_t uptimeS = 0;
};

void uiBegin();
void uiSetModel(const UiModel& m);   // redraws current screen
// The delivered panel has no touch controller; screens are switched with
// 'd' over the serial monitor (future: push button on the freed D0 pin).
void uiToggleScreen();
