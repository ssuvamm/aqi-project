# Plan — Portable AQI Accessory + Flutter App (revised 2026-07-17)

## Product vision

A **portable air-quality accessory**: whoever has the device with them can
check it from their own phone — the Flutter app connects over **Bluetooth**
and shows live + historical air data. Access model = physical possession of
the device, not tied to one person's phone. The TFT screen stays for now but is treated as an
*optional* module — the device must work fully headless so the screen can be
removed later.

## Decisions (supersedes the 2026-07-17 WiFi plan)

| Question | Decision | Why |
|---|---|---|
| Board | **ESP32-C3** (recommend the tiny C3 SuperMini, ~₹200; classic ESP32 DevKit is the more-pins fallback) | ESP8266 has no Bluetooth. C3 adds BLE 5 + two hardware UARTs + 400 KB RAM; SuperMini size suits a portable accessory. |
| Transport | **BLE-first** (was: WiFi) | Portable = no router dependency. Whoever is carrying the device connects from their phone. WiFi becomes an optional "home dock" mode much later. |
| Phone OS | Android first, iOS-compatible architecture | Unchanged. BLE package choice must be cross-platform (flutter_blue_plus). |
| Log depth | 1-min averages, ~90 days in LittleFS | Unchanged. |
| Time source | **The phone**, over BLE (was: NTP) | Portable device has no internet. App sets the device clock on every connect; samples logged before any time-sync are dropped. |
| Screen | Keep, but firmware must run headless | Display becomes a compile-time/runtime-optional module. |
| Power (v1) | USB power bank | Zero design work. Battery+boost design is a later phase — note the 5 V ±0.1 V MH-Z19E requirement makes this non-trivial. |

Architecture in one sentence: the device is a self-contained BLE sensor +
logger; the Flutter app is its main display, syncing history into a phone-side
SQLite archive whenever any phone connects.

**What the C3 kills from the old plan:** the WiFi-vs-SoftwareSerial soak test
(both sensors get *hardware* UARTs — SoftwareSerial and the take-turns state
machine are deleted), and the ESP8266 heap worry.

---

## Phase 0 — Procurement & groundwork

1. **Order the board** (decide: C3 SuperMini for size vs classic ESP32 DevKit
   for pin headroom). Nothing blocks on this except delivery.
2. **Flutter toolchain check** on this PC: `flutter doctor`, Android SDK,
   phone in developer mode, hello-world APK installed. Can happen while the
   board ships.

## Phase 1 — Board migration (firmware v0.3)

- New wiring (fresh WIRING.md): both sensors on hardware UARTs, SPI TFT,
  button on a normal pull-up pin (GPIO16 quirk gone). **Also wire PMS5003
  pin 3 (SET) to a GPIO this time** — fan sleep control, the key to battery
  life later. Route around C3 strapping pins.
- PlatformIO env `espressif32`; port pin map + TFT_eSPI flags; delete
  SoftwareSerial scheduling (sensors now read concurrently); logging over
  native USB.
- Refactor: display becomes an optional module behind a build flag (headless
  build must compile and run).
- ✅ Milestone: feature parity with v0.2.2 on the new board.

## Phase 2 — BLE service (firmware v0.4)

- BLE GATT design (NimBLE stack), device advertises as `AQI-xxxx`:
  - **Live characteristic** — notify each 5 s cycle (readings + AQI + status)
  - **Info characteristic** — firmware version, uptime, log span, battery/power state
  - **Time-sync characteristic** — phone writes epoch time on connect
  - **History characteristic** — chunked transfer protocol for log backfill
- No pairing/auth in v1: access is by physical possession — whoever is
  carrying the device connects with the app. (Technically BLE range extends a
  few meters beyond the carrier; acceptable for read-only data. Pairing gets
  added when control characteristics ship in Phase 6.)
- ✅ Milestone: a generic BLE scanner app (nRF Connect) shows live values.

## Phase 3 — On-device logging (firmware v0.5)

- Same design as before: 1-min averages → daily LittleFS files, ~90 days,
  oldest-day eviction. Timestamps only after a phone has set the clock;
  unsynced samples dropped.
- ✅ Milestone: nRF Connect / debug USB dump shows a day of stored samples.

## Phase 4 — Flutter app MVP (Android)

- Stack: `flutter_blue_plus` (cross-platform BLE), `riverpod`, `drift`
  (SQLite), `fl_chart`. Handle the Android BLE permission dance
  (location/nearby-devices) early — it's the usual first wall.
- Screens: **scan/connect** (find any AQI-xxxx nearby, remember favorites)
  and **live dashboard** mirroring the TFT design (AQI card + category color,
  CO2 card, PM tiles, warm-up/status).
- On connect: app writes time-sync automatically.
- ✅ Milestone: live readings on the phone over BLE, screen-free operation
  demonstrated (TFT covered/ignored).

## Phase 5 — History sync & charts

- Sync engine: on connect, fetch history since last synced timestamp into
  SQLite. BLE throughput is ~5–20 kB/s — fine for incremental gaps (hours to
  a few days); a full 90-day backfill may take minutes, show progress UI.
- Charts: day/week/month for PM2.5, PM10, CO2, AQI (min/avg/max); CSV export.
- Multi-phone reality: one connected central at a time; each phone keeps its
  own archive and syncs whatever gap it personally has.
- ✅ Milestone: gap-free week chart on a phone that was away for days.

## Phase 6 — Later / parked

- **Battery power**: LiPo + boost to a clean 5 V (MH-Z19E needs 5.0 V ±0.1 V),
  PMS5003 fan duty-cycling via the now-wired SET pin (fan is ~100 mA — the
  single biggest battery lever), battery gauge on Info characteristic.
- Screen removal decision (headless build already exists from Phase 1).
- Threshold notifications; device control from app (zero-cal, ABC, backlight).
- Optional WiFi "home dock" mode → MQTT/Home Assistant.
- iOS build (needs a Mac).

## Known risks & constraints

| Risk | Mitigation |
|---|---|
| Flutter BLE is the fiddly part (permissions, OS quirks, reconnects) | Use flutter_blue_plus, tackle permissions in Phase 4 week 1, keep nRF Connect as the reference client to isolate app-vs-firmware bugs |
| BLE throughput for large backfills | Incremental sync is the normal case; progress UI for the rare full pull |
| No auth on BLE in v1 | Access-by-possession is the intent; read-only exposure beyond arm's reach is accepted. Pairing becomes mandatory when control characteristics ship (Phase 6) |
| Portable power: MH-Z19E's strict 5 V ±0.1 V | Power bank in v1; proper boost-converter design deferred to Phase 6 |
| C3 SuperMini pin count (~13 usable) | Budget: 3 UART + 4 SPI/TFT + button + PMS-SET = ~9. Fits; verify against strapping pins during Phase 1 wiring doc |
