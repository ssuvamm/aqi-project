#include "ui.h"
#include <TFT_eSPI.h>
#include "aqi.h"

// Landscape 320x240, card-based layout. Two screens:
//   DASHBOARD — AQI card + CO2 card (each with a colored category pill),
//               an AQI gauge bar with position marker, three PM tiles
//   DETAILS   — full readout table (particle counts, temps, uptime)
// The panel has no touch controller; main.cpp calls uiToggleScreen() on a
// serial command. Dynamic fields draw with text padding over their card
// color so they erase their own previous value — no flicker.

namespace {
TFT_eSPI tft;

enum class Screen : uint8_t { DASHBOARD, DETAILS };
Screen screen = Screen::DASHBOARD;
bool layoutDrawn = false;
UiModel model;
bool haveModel = false;

// palette
constexpr uint16_t COL_BG     = TFT_BLACK;
constexpr uint16_t COL_HEAD   = 0x18E3;   // header strip
constexpr uint16_t COL_CARD   = 0x1082;   // card background
constexpr uint16_t COL_BORDER = 0x39E7;
constexpr uint16_t COL_LABEL  = 0xAD55;   // mid grey
constexpr uint16_t COL_VALUE  = 0xEF7D;   // near white

// geometry
constexpr int HEAD_H = 24;
constexpr int CARD_Y = 30, CARD_H = 112, CARD_W = 152;
constexpr int AQI_X = 6, CO2_X = 162;
constexpr int GX = 7, GY = 148, GW = 306, GH = 10, GSEG = GW / 6;  // gauge
constexpr int MARK_Y = GY + GH + 5, MARK_H = 8;  // below the gauge border
constexpr int TILE_Y = 174, TILE_H = 60, TILE_W = 98;
constexpr int TILE_X[3] = {6, 111, 216};

const uint16_t GAUGE_COLS[6] = {TFT_GREEN, TFT_YELLOW, TFT_ORANGE,
                                TFT_RED, TFT_PURPLE, TFT_MAROON};

void drawCardFrame(int x, int y, int w, int h) {
    tft.fillRoundRect(x, y, w, h, 8, COL_CARD);
    tft.drawRoundRect(x, y, w, h, 8, COL_BORDER);
}

// Colored capsule with black text, centered in the lower part of a card.
void drawPill(int cardX, const char* txt, uint16_t color) {
    const int x = cardX + 12, y = CARD_Y + CARD_H - 34, w = CARD_W - 24, h = 24;
    tft.fillRoundRect(x, y, w, h, 12, color);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(0);
    tft.setTextColor(TFT_BLACK, color);
    tft.drawString(txt, x + w / 2, y + h / 2 + 1, 2);
}

// Map an AQI value to an x position on the gauge (each category = 1 segment).
int aqiToGaugeX(int aqi) {
    static const int lo[6] = {0, 51, 101, 151, 201, 301};
    static const int hi[6] = {50, 100, 150, 200, 300, 500};
    aqi = constrain(aqi, 0, 500);
    for (int i = 0; i < 6; i++) {
        if (aqi <= hi[i])
            return GX + i * GSEG + (aqi - lo[i]) * GSEG / (hi[i] - lo[i]);
    }
    return GX + GW;
}

void drawHeader(const char* title) {
    tft.fillRect(0, 0, 320, HEAD_H, COL_HEAD);
    tft.setTextDatum(ML_DATUM);
    tft.setTextColor(COL_VALUE, COL_HEAD);
    tft.setTextPadding(0);
    tft.drawString(title, 8, HEAD_H / 2 + 1, 2);
}

void drawStatus() {
    const char* txt;
    uint16_t col;
    if (model.pmWarmup || model.co2Warmup) { txt = "WARM-UP"; col = TFT_YELLOW; }
    else if (!model.pmValid || !model.co2Valid) { txt = "NO DATA"; col = TFT_RED; }
    else { txt = "LIVE"; col = TFT_GREEN; }

    tft.fillCircle(306, HEAD_H / 2, 5, col);
    tft.setTextDatum(MR_DATUM);
    tft.setTextColor(col, COL_HEAD);
    tft.setTextPadding(tft.textWidth("WARM-UP", 2));
    tft.drawString(txt, 296, HEAD_H / 2 + 1, 2);
    tft.setTextPadding(0);
}

// ---------------------------------------------------------------- dashboard

void drawDashboardLayout() {
    tft.fillScreen(COL_BG);
    drawHeader("AIR QUALITY");

    drawCardFrame(AQI_X, CARD_Y, CARD_W, CARD_H);
    drawCardFrame(CO2_X, CARD_Y, CARD_W, CARD_H);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_LABEL, COL_CARD);
    tft.drawString("US AQI", AQI_X + 12, CARD_Y + 8, 2);
    tft.drawString("CO2 ppm", CO2_X + 12, CARD_Y + 8, 2);

    // gauge: six fixed category segments in a 2px white frame (1px inset)
    tft.drawRect(GX - 3, GY - 3, GW + 6, GH + 6, TFT_WHITE);
    tft.drawRect(GX - 2, GY - 2, GW + 4, GH + 4, TFT_WHITE);
    for (int i = 0; i < 6; i++)
        tft.fillRect(GX + i * GSEG, GY, GSEG - 1, GH, GAUGE_COLS[i]);

    for (int i = 0; i < 3; i++) {
        drawCardFrame(TILE_X[i], TILE_Y, TILE_W, TILE_H);
    }
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(COL_LABEL, COL_CARD);
    tft.drawString("PM1.0", TILE_X[0] + TILE_W / 2, TILE_Y + 7, 2);
    tft.drawString("PM2.5", TILE_X[1] + TILE_W / 2, TILE_Y + 7, 2);
    tft.drawString("PM10", TILE_X[2] + TILE_W / 2, TILE_Y + 7, 2);
}

void drawGaugeMarker() {
    tft.fillRect(GX - 6, MARK_Y, GW + 12, MARK_H, COL_BG);  // erase old marker
    if (!(model.pmValid && model.aqi >= 0)) return;
    int x = aqiToGaugeX(model.aqi);
    tft.fillTriangle(x, MARK_Y, x - 5, MARK_Y + MARK_H - 1, x + 5, MARK_Y + MARK_H - 1,
                     TFT_WHITE);
}

void drawDashboard() {
    drawStatus();

    // ---- AQI card ----
    tft.setTextDatum(TC_DATUM);
    tft.setTextPadding(120);
    if (model.pmValid && model.aqi >= 0) {
        AqiCategory cat = aqiCategory(model.aqi);
        tft.setTextColor(COL_VALUE, COL_CARD);
        tft.drawNumber(model.aqi, AQI_X + CARD_W / 2, CARD_Y + 28, 6);
        drawPill(AQI_X, aqiCategoryName(cat), aqiCategoryColor(cat));
    } else {
        tft.setTextColor(COL_LABEL, COL_CARD);
        tft.drawString("--", AQI_X + CARD_W / 2, CARD_Y + 28, 6);
        drawPill(AQI_X, model.pmWarmup ? "WARM-UP" : "NO DATA", COL_LABEL);
    }

    // ---- CO2 card ----
    tft.setTextDatum(TC_DATUM);
    tft.setTextPadding(130);
    if (model.co2Valid && model.co2 >= 0) {
        tft.setTextColor(COL_VALUE, COL_CARD);
        tft.drawNumber(min(model.co2, 9999), CO2_X + CARD_W / 2, CARD_Y + 28, 6);
        drawPill(CO2_X, co2Label(model.co2), co2Color(model.co2));
    } else {
        tft.setTextColor(COL_LABEL, COL_CARD);
        tft.drawString("--", CO2_X + CARD_W / 2, CARD_Y + 28, 6);
        drawPill(CO2_X, model.co2Warmup ? "WARM-UP" : "NO DATA", COL_LABEL);
    }

    drawGaugeMarker();

    // ---- PM tiles (each value colored by its own sub-index where defined) ----
    tft.setTextDatum(TC_DATUM);
    tft.setTextPadding(84);
    if (model.pmValid) {
        tft.setTextColor(COL_VALUE, COL_CARD);
        tft.drawNumber(model.pm1, TILE_X[0] + TILE_W / 2, TILE_Y + 26, 4);
        tft.setTextColor(aqiCategoryColor(aqiCategory(model.aqiPm25)), COL_CARD);
        tft.drawNumber(model.pm25, TILE_X[1] + TILE_W / 2, TILE_Y + 26, 4);
        tft.setTextColor(aqiCategoryColor(aqiCategory(model.aqiPm10)), COL_CARD);
        tft.drawNumber(model.pm10, TILE_X[2] + TILE_W / 2, TILE_Y + 26, 4);
    } else {
        tft.setTextColor(COL_LABEL, COL_CARD);
        for (int i = 0; i < 3; i++)
            tft.drawString("--", TILE_X[i] + TILE_W / 2, TILE_Y + 26, 4);
    }
    tft.setTextPadding(0);
}

// ------------------------------------------------------------------ details

void drawDetailsLayout() {
    tft.fillScreen(COL_BG);
    drawHeader("DETAILS  ('d' = back)");

    static const char* labels[] = {
        "PM1.0 ug/m3", "PM2.5 ug/m3", "PM10 ug/m3",
        ">0.3um /0.1L", ">0.5um /0.1L", ">1.0um /0.1L",
        ">2.5um /0.1L", ">5.0um /0.1L", ">10um /0.1L",
        "CO2 ppm", "Sensor temp C",
        "AQI PM2.5 / PM10", "Uptime",
    };
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_LABEL, COL_BG);
    for (uint8_t i = 0; i < 13; i++)
        tft.drawString(labels[i], 8, 32 + i * 16, 2);
}

void drawDetailRow(uint8_t row, const String& value, uint16_t color = COL_VALUE) {
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(color, COL_BG);
    tft.setTextPadding(140);
    tft.drawString(value, 312, 32 + row * 16, 2);
    tft.setTextPadding(0);
}

void drawDetails() {
    drawStatus();
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
        for (uint8_t i = 0; i <= 8; i++) drawDetailRow(i, na, COL_LABEL);
    }

    if (model.co2Valid) {
        drawDetailRow(9, String(model.co2), co2Color(model.co2));
        drawDetailRow(10, String(model.temp));
    } else {
        drawDetailRow(9, na, COL_LABEL);
        drawDetailRow(10, na, COL_LABEL);
    }

    if (model.pmValid)
        drawDetailRow(11, String(model.aqiPm25) + " / " + String(model.aqiPm10));
    else
        drawDetailRow(11, na, COL_LABEL);

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
    tft.setRotation(1);   // landscape, USB connector of the D1 mini to the right
    render();
}

void uiSetModel(const UiModel& m) {
    model = m;
    haveModel = true;
    render();
}

void uiToggleScreen() {
    screen = (screen == Screen::DASHBOARD) ? Screen::DETAILS : Screen::DASHBOARD;
    layoutDrawn = false;
    render();
}
