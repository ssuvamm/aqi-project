#pragma once
#include <Arduino.h>

// ============================================================================
// Pin assignments (GPIO numbers; D-labels are the D1 mini silkscreen)
// TFT + touch pins are defined in platformio.ini build_flags (TFT_eSPI
// requires them at compile time). Full wiring table: docs/WIRING.md
// ============================================================================
constexpr int PIN_PMS_RX = 4;   // D2  <- PMS5003 pin 5 (TXD)
constexpr int PIN_MHZ_RX = 5;   // D1  <- MH-Z19E Tx
constexpr int PIN_MHZ_TX = 2;   // D4  -> MH-Z19E Rx (also onboard LED: blinks on each CO2 poll)
// Screen-toggle push button between D0 and 3V3. GPIO16 has only an internal
// PULL-DOWN (no pull-up), so the button must feed 3V3 in; pressed = HIGH.
constexpr int PIN_BUTTON = 16;  // D0

// ============================================================================
// Serial settings
// ============================================================================
constexpr uint32_t DEBUG_BAUD = 115200;
constexpr uint32_t PMS_BAUD   = 9600;   // fixed by sensor
constexpr uint32_t MHZ_BAUD   = 9600;   // fixed by sensor

// ============================================================================
// Timing
// ============================================================================
constexpr uint32_t SAMPLE_INTERVAL_MS     = 5000;   // one full sensor cycle
constexpr uint32_t PMS_LISTEN_TIMEOUT_MS  = 3500;   // PMS frames arrive every ~2.3 s (stable mode)
constexpr uint32_t MHZ_RESPONSE_TIMEOUT_MS = 500;   // MH-Z19E answers within ~50 ms
constexpr uint32_t DATA_STALE_MS          = 30000;  // show "--" if no valid reading for this long

// Sensor warm-up (from datasheets: PMS5003 needs ~30 s after fan start,
// MH-Z19E preheat time is 1 min)
constexpr uint32_t PMS_WARMUP_MS = 30000;
constexpr uint32_t MHZ_WARMUP_MS = 60000;

// ============================================================================
// MH-Z19E options
// ============================================================================
// ABC = Automatic Baseline Correction. The sensor assumes it sees fresh air
// (400 ppm) at least once per 24 h and self-calibrates to it. Good for homes
// that get ventilated daily; disable for greenhouses/24h-occupied rooms.
constexpr bool MHZ19_ABC_ENABLED = true;
