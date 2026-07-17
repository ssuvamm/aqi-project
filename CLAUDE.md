# AQI Monitor — ESP8266 + PMS5003 + MH-Z19E + ILI9341

Indoor air-quality monitor: PM1.0/PM2.5/PM10 + US AQI + CO2 on a 2.4" touch TFT.
No networking yet (planned). Owner: Suvam.

## Hardware (exact models)

| Part | Model | Datasheet |
|---|---|---|
| MCU board | ESP8266 D1 Mini V2 (CH340, 4 MB flash) | `docs/datasheets/ESP8266EX_datasheet.pdf` |
| PM sensor | Plantower PMS5003 (UART 9600, active mode) | `docs/datasheets/PMS5003_datasheet.pdf` |
| CO2 sensor | Winsen MH-Z19E (NDIR, UART 9600) | `docs/datasheets/MH-Z19E_datasheet.pdf` |
| Display | 2.4" SPI 240×320, **non-touch, ST7789V** (robu listing claimed ILI9341+touch; delivered unit differs) | no PDF exists; pinout in `docs/WIRING.md` |

**Pin map lives in two places (keep in sync):** sensor UART pins in
`include/config.h`, TFT/touch pins in `platformio.ini` build_flags (TFT_eSPI
needs compile-time defines). Full human-readable table: `docs/WIRING.md`.
Every GPIO on the D1 Mini is allocated — see WIRING.md §5 before adding hardware.

## Build / flash / monitor

PlatformIO CLI (not on PATH — use full path or the VS Code extension):

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run              # build
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload    # flash
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor   # logs, 115200
```

Details + troubleshooting: `docs/FLASHING.md`.

## Architecture

```
src/main.cpp     state machine: PMS_LISTEN → CO2_WAIT → IDLE (5 s cycle), owns raw readings
src/pms5003.*    PMS5003 frame parser (32-byte active-mode frames, checksummed)
src/mhz19.*      MH-Z19 family UART protocol (0x86 read, 0x79 ABC, 0x87 zero-cal)
src/aqi.*        US EPA AQI math (2024 PM2.5 breakpoints) + CO2 comfort bands + colors
src/ui.*         TFT_eSPI rendering; 2 screens (dashboard/details), 'd' on serial toggles
include/config.h pins, baud rates, timing constants, feature flags
```

### Non-negotiable design constraints

- **Both sensors are on SoftwareSerial** (hardware UART stays on USB). Bit-banged
  RX is reliable for only ONE port at a time → the main-loop state machine takes
  turns (`enableRx`). Never listen to both simultaneously.
- **Never draw on the TFT while PMS_LISTEN is active** — long SPI bursts break
  SoftwareSerial timing. UI updates happen only in `publishReadings()` at the
  end of a cycle (and on touch, which is tolerable).
- Boot-strap pins are all safely occupied (D3=TFT DC, D4=UART TX idle-high,
  D8=TFT CS). Don't repurpose them without checking WIRING.md §5.
- **The delivered display is NON-touch and behaves as ST7789V**, not the listed
  ILI9341 (ILI9341 driver symptoms: ignored rotation + 80px noise band +
  red/blue swap). Default pio env `d1_mini` = ST7789; `d1_mini_ili9341` is the
  fallback for a future genuine ILI9341. This unit also needs
  `TFT_INVERSION_OFF` + `TFT_RGB_ORDER=TFT_BGR` (photo-verified 2026-07-17).
- Screen-toggle button on D0 wires to **3V3** (GPIO16 has only a pull-down;
  HIGH = pressed). D6 (MISO) is unwired/claimable; A0 free.
- TFT SDO is not wired; never enable TFT reads.
- Sensors reject bad data by checksum; a missed frame just means the previous
  value stays until `DATA_STALE_MS`.

## Conventions

- Every user-visible change gets an entry in `CHANGES.md` (Keep-a-Changelog style).
- Config knobs go in `include/config.h`, not scattered as magic numbers.
- New display pins / driver flags go in `platformio.ini`, never in library files
  (`USER_SETUP_LOADED` pattern — the TFT_eSPI library itself stays pristine).
- Datasheets for any new hardware get downloaded to `docs/datasheets/`.

## Roadmap

Current: v0.2.x — local display, screen switch via D0 button or serial 'd'.

**The agreed forward plan lives in `docs/PLAN.md`** (revised 2026-07-17):
product vision = **portable BLE accessory**, screen optional/removable.
Migrate to **ESP32-C3** (kills SoftwareSerial — both sensors get hardware
UARTs), BLE GATT service (live notify + time-sync-from-phone + chunked
history), 1-min/90-day LittleFS log, Flutter Android app (flutter_blue_plus,
SQLite archive, charts). Wire PMS5003 SET pin during the C3 rewire (battery
lever). The ESP8266 WiFi plan is superseded; WiFi returns only as an optional
"home dock" mode later.
