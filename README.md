# AQI Monitor

Indoor air-quality monitor built on an **ESP8266 D1 Mini V2**:

- **PMS5003** — PM1.0 / PM2.5 / PM10 laser particle sensor
- **MH-Z19E** — NDIR CO2 sensor
- **2.4" SPI TFT (ST7789, non-touch)** — color-coded US-AQI dashboard;
  push button (D0) or `d` over serial switches to a detailed readout

| Doc | What's in it |
|---|---|
| [docs/PLAN.md](docs/PLAN.md) | roadmap: ESP32-C3 + BLE portable accessory + Flutter app |
| [docs/UI.md](docs/UI.md) | what both screens show and what it means |
| [docs/WIRING.md](docs/WIRING.md) | complete pin-by-pin wiring tables + diagram |
| [docs/FLASHING.md](docs/FLASHING.md) | build, flash, monitor, troubleshooting |
| [CLAUDE.md](CLAUDE.md) | architecture, design constraints, roadmap |
| [CHANGES.md](CHANGES.md) | changelog |
| [docs/datasheets/](docs/datasheets/) | PMS5003, MH-Z19E, ESP8266EX PDFs |

Quick start: wire per WIRING.md, then

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload
```
