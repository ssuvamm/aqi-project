# Changelog

All notable changes to the AQI Monitor project. Format loosely follows
[Keep a Changelog](https://keepachangelog.com/); versions are firmware versions.

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
