#include "ui.h"
#include <TFT_eSPI.h>
#include "aqi.h"

// Landscape 320x240. Two screens:
//   DASHBOARD — big AQI (left) + big CO2 (right) + PM strip (bottom)
//   DETAILS   — full readout table (particle counts, temps, uptime)
// Tap anywhere to toggle. Dynamic fields are drawn with text padding so they
// erase their own previous value — no full-screen clears, no flicker.

namespace {
TFT_eSPI tft;

enum class Screen : uint8_t { DASHBOARD, DETAILS };
Screen screen = Screen::DASHBOARD;
bool layoutDrawn = false;
UiModel model;
bool haveModel = false;
uint32_t lastTouchMs = 0;

constexpr uint16_t GRID = 0x39E7;      // dark grey lines
constexpr uint16_t LABEL = TFT_LIGHTGREY;

void drawDashboardLayout() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(LABEL, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("AIR QUALITY MONITOR", 6, 6, 2);

    tft.drawFastHLine(0, 26, 320, GRID);
    tft.drawFastVLine(160, 26, 148, GRID);
    tft.drawFastHLine(0, 174, 320, GRID);

    tft.setTextDatum(TC_DATUM);
    tft.drawString("US AQI", 80, 32, 2);
    tft.drawString("CO2  ppm", 240, 32, 2);
    tft.drawString("PM1.0", 53, 182, 2);
    tft.drawString("PM2.5", 160, 182, 2);
    tft.drawString("PM10", 267, 182, 2);
    tft.setTextDatum(BC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("ug/m3  -  tap for details", 160, 238, 2);
}

void drawStatus() {
    // top-right status word
    const char* txt;
    uint16_t col;
    if (model.pmWarmup || model.co2Warmup) { txt = "WARM-UP"; col = TFT_YELLOW; }
    else if (!model.pmValid || !model.co2Valid) { txt = "NO DATA"; col = TFT_RED; }
    else { txt = "LIVE"; col = TFT_GREEN; }
    tft.setTextDatum(TR_DATUM);
    tft.setTextPadding(tft.textWidth("WARM-UP", 2));
    tft.setTextColor(col, TFT_BLACK);
    tft.drawString(txt, 314, 6, 2);
    tft.setTextPadding(0);
}

void drawDashboard() {
    drawStatus();

    // ---- AQI (left cell) ----
    tft.setTextDatum(TC_DATUM);
    tft.setTextPadding(150);
    if (model.pmValid && model.aqi >= 0) {
        AqiCategory cat = aqiCategory(model.aqi);
        tft.setTextColor(aqiCategoryColor(cat), TFT_BLACK);
        tft.drawNumber(model.aqi, 80, 62, 6);
        tft.setTextPadding(156);
        tft.drawString(aqiCategoryName(cat), 80, 130, 4);
    } else {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("--", 80, 62, 6);
        tft.setTextPadding(156);
        tft.drawString(model.pmWarmup ? "WAIT" : "NO PM", 80, 130, 4);
    }

    // ---- CO2 (right cell) ----
    tft.setTextPadding(150);
    if (model.co2Valid && model.co2 >= 0) {
        tft.setTextColor(co2Color(model.co2), TFT_BLACK);
        tft.drawNumber(model.co2, 240, 62, 6);
        tft.setTextPadding(156);
        tft.drawString(co2Label(model.co2), 240, 130, 4);
    } else {
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawString("--", 240, 62, 6);
        tft.setTextPadding(156);
        tft.drawString(model.co2Warmup ? "WAIT" : "NO CO2", 240, 130, 4);
    }

    // ---- PM strip (bottom) ----
    tft.setTextColor(model.pmValid ? TFT_WHITE : TFT_DARKGREY, TFT_BLACK);
    tft.setTextPadding(90);
    if (model.pmValid) {
        tft.drawNumber(model.pm1, 53, 204, 4);
        tft.drawNumber(model.pm25, 160, 204, 4);
        tft.drawNumber(model.pm10, 267, 204, 4);
    } else {
        tft.drawString("--", 53, 204, 4);
        tft.drawString("--", 160, 204, 4);
        tft.drawString("--", 267, 204, 4);
    }
    tft.setTextPadding(0);
}

void drawDetailsLayout() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(LABEL, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    tft.drawString("DETAILS", 6, 6, 2);
    tft.setTextDatum(TR_DATUM);
    tft.drawString("tap to go back", 314, 6, 2);
    tft.drawFastHLine(0, 26, 320, GRID);

    static const char* labels[] = {
        "PM1.0 ug/m3", "PM2.5 ug/m3", "PM10 ug/m3",
        ">0.3um /0.1L", ">0.5um /0.1L", ">1.0um /0.1L",
        ">2.5um /0.1L", ">5.0um /0.1L", ">10um /0.1L",
        "CO2 ppm", "Sensor temp C",
        "AQI PM2.5 / PM10", "Uptime",
    };
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(LABEL, TFT_BLACK);
    for (uint8_t i = 0; i < 13; i++)
        tft.drawString(labels[i], 8, 32 + i * 16, 2);
}

void drawDetailRow(uint8_t row, const String& value, uint16_t color = TFT_WHITE) {
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(color, TFT_BLACK);
    tft.setTextPadding(140);
    tft.drawString(value, 312, 32 + row * 16, 2);
    tft.setTextPadding(0);
}

void drawDetails() {
    const String na = "--";
    if (model.pmValid) {
        drawDetailRow(0, String(model.pm1));
        drawDetailRow(1, String(model.pm25));
        drawDetailRow(2, String(model.pm10));
        drawDetailRow(3, String(model.n03));
        drawDetailRow(4, String(model.n05));
        drawDetailRow(5, String(model.n10));
        drawDetailRow(6, String(model.n25));
        drawDetailRow(7, String(model.n50));
        drawDetailRow(8, String(model.n100));
    } else {
        for (uint8_t i = 0; i <= 8; i++) drawDetailRow(i, na, TFT_DARKGREY);
    }

    if (model.co2Valid) {
        drawDetailRow(9, String(model.co2), co2Color(model.co2));
        drawDetailRow(10, String(model.temp));
    } else {
        drawDetailRow(9, na, TFT_DARKGREY);
        drawDetailRow(10, na, TFT_DARKGREY);
    }

    if (model.pmValid)
        drawDetailRow(11, String(model.aqiPm25) + " / " + String(model.aqiPm10));
    else
        drawDetailRow(11, na, TFT_DARKGREY);

    uint32_t s = model.uptimeS;
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu:%02lu:%02lu",
             (unsigned long)(s / 3600), (unsigned long)(s / 60 % 60), (unsigned long)(s % 60));
    drawDetailRow(12, buf);
}

void render() {
    if (!layoutDrawn) {
        if (screen == Screen::DASHBOARD) drawDashboardLayout();
        else drawDetailsLayout();
        layoutDrawn = true;
    }
    if (!haveModel) return;
    if (screen == Screen::DASHBOARD) drawDashboard();
    else drawDetails();
}
}  // namespace

void uiBegin() {
    tft.init();
    tft.setRotation(1);   // USB connector of the D1 mini pointing right
    render();
}

void uiSetModel(const UiModel& m) {
    model = m;
    haveModel = true;
    render();
}

void uiHandleTouch() {
    uint32_t now = millis();
    if (now - lastTouchMs < 400) return;   // debounce
    // Raw pressure read needs no XY calibration — any firm tap registers.
    if (tft.getTouchRawZ() < 500) return;
    lastTouchMs = now;
    screen = (screen == Screen::DASHBOARD) ? Screen::DETAILS : Screen::DASHBOARD;
    layoutDrawn = false;
    render();
}
