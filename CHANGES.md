# Changelog

All notable changes to the AQI Monitor project. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/); versions are firmware versions.

## [0.2.2] — 2026-07-17

### Changed
- AQI gauge now has a 2 px white frame (1 px black inset); marker moved 3 px
  down to clear it.

## [0.2.1] — 2026-07-17

### Fixed
- ST7789 colors: this panel needs `TFT_INVERSION_OFF` + `TFT_RGB_ORDER=TFT_BGR`
  (photo showed photo-negative with red/blue swap: black→light grey, green
  FRESH pill→purple, red gauge→yellow).

### Added
- Screen-toggle push button on D0 (wired **D0 ↔ 3V3**; GPIO16 only has an
  internal pull-down, so HIGH = pressed). Serial `d` still works.
- `docs/UI.md` — explanation of both screens, all thresholds and indicators.
- FLASHING.md §3 expanded: device-monitor how-to, `d` command, FTDI-style
  COM6 naming, LED heartbeat, boot-ROM garbage note.

## [0.2.0] — 2026-07-17

### Fixed
- **Display driver**: the delivered 2.4" panel is the non-touch variant with an
  ST7789V controller, not the listed ILI9341+XPT2046. Driving it as ILI9341
  ignored rotation (80 px band of unwritten-GRAM noise) and swapped red/blue.
  Default PlatformIO env now uses `ST7789_DRIVER`; `d1_mini_ili9341` env kept
  as fallback.

### Changed
- Removed all touch support (no XPT2046 on the panel; polling a floating MISO
  risked phantom screen toggles). Dashboard/details switching now via `d` on
  the serial monitor; a push button on the freed D0 pin is on the roadmap.
- UI redesign: header bar with status dot, card-based layout, black-on-color
  category pills (AQI category + CO2 comfort), six-segment AQI gauge bar with
  white position marker, PM tiles with per-pollutant sub-index coloring.
- `upload_port = COM6` pinned in platformio.ini.
- WIRING.md/CLAUDE.md updated: touch pins removed, D0 and D6 now free.

## [0.1.0] — 2026-07-17

### Added
- Initial PlatformIO project scaffold (ESP8266 D1 Mini V2, Arduino framework).
- PMS5003 driver: active-mode 32-byte frame parser with checksum validation
  (`src/pms5003.*`) — PM1.0/2.5/10 (standard + atmospheric) and 6 particle-count bins.
- MH-Z19E driver: Winsen 9-byte UART protocol (`src/mhz19.*`) — CO2 read,
  ABC auto-calibration on/off, zero-point calibration hook (unused in v1).
- US EPA AQI computation with May-2024 revised PM2.5 breakpoints + PM10
  breakpoints; combined AQI = max of sub-indices (`src/aqi.*`).
- TFT UI on ILI9341 (TFT_eSPI, configured via platformio.ini build flags):
  dashboard screen (color-coded AQI + CO2, PM strip) and details screen
  (all raw values, particle counts, sensor temp, uptime). XPT2046 touch:
  tap toggles screens (raw pressure detect, no calibration needed).
- Sensor scheduling state machine: alternating SoftwareSerial RX windows
  (PMS listen 3.5 s → CO2 request/wait 0.5 s → idle), 5 s full cycle;
  display only redrawn between RX windows.
- Warm-up handling (PMS 30 s, MH-Z19E 60 s per datasheets) and 30 s staleness
  fallback ("--" + NO DATA status).
- WiFi explicitly disabled in v1 (power/heat).
- Docs: `docs/WIRING.md` (full pin tables + boot-strap analysis + ASCII diagram),
  `docs/FLASHING.md` (CH340 driver, build/upload/monitor, troubleshooting).
- Datasheets downloaded to `docs/datasheets/` (PMS5003 V2.3 manual, MH-Z19E
  user's manual v1.0, ESP8266EX datasheet). The 2.4" TFT module has no PDF
  datasheet (documented in WIRING.md instead).
- `CLAUDE.md` (project conventions/architecture) and this changelog.
