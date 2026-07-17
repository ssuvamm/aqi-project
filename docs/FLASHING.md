# Flashing Guide — ESP8266 D1 Mini V2

## 0. One-time setup

1. **CH340 USB driver** — the D1 Mini V2 uses a CH340G USB-serial chip.
   Windows 11 usually installs it automatically. Check: plug the board in,
   open Device Manager → *Ports (COM & LPT)* → you should see
   **"USB-SERIAL CH340 (COMx)"**. If it shows an unknown device, install the
   driver from https://www.wch-ic.com/downloads/CH341SER_EXE.html
2. **PlatformIO** — already installed on this PC
   (`%USERPROFILE%\.platformio\penv\Scripts\pio.exe`). The VS Code PlatformIO
   extension gives you the same commands as toolbar buttons.

## 1. Build

```powershell
cd "d:\Suvam\Documents\claude-space\aqi project"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run
```

First build downloads the ESP8266 platform + toolchain (~200 MB, a few minutes).

## 2. Flash

Plug in the D1 Mini (no BOOT buttons needed — the board auto-enters flash mode
via its USB circuitry):

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -t upload
```

PlatformIO auto-detects the COM port. To pin it explicitly, add
`upload_port = COM5` (your port) to `platformio.ini`.

## 3. Watch the logs (device monitor)

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor
```

(or the plug icon in the VS Code PlatformIO toolbar). 115200 baud, port COM6
is pinned in platformio.ini. Expected output, one line per 5 s cycle:

```
AQI Monitor (PMS5003 + MH-Z19E)
MH-Z19E ABC auto-calibration: on
[     5s] PM1=3 PM2.5=5 PM10=6 ug/m3 | CO2=612 ppm T=23 C | AQI=21 ...
```

Things to know:

- **Type `d` in the monitor** to switch the TFT between dashboard and details
  (same as pressing the D0 push button).
- Press `Ctrl+C` to exit. **The monitor must be closed before flashing again**
  (it holds the COM port).
- This board enumerates as **"USB Serial Port (COM6)"** — an FTDI-style name,
  not the usual "USB-SERIAL CH340"; this D1 Mini clone uses a different
  USB-serial chip. Works identically.
- The onboard blue LED blinks every 5 s while running — that's the CO2 poll
  sharing GPIO2. Blinking = firmware alive.
- The garbled characters right after reset are the ESP8266 boot ROM printing
  at 74880 baud — normal, ignore them.

## 4. First-boot expectations

| Time after power-up | What you'll see |
|---|---|
| 0–2 s | TFT lights up, dashboard grid appears, status **WARM-UP** |
| ~30 s | PM values + AQI go live |
| ~60 s | CO2 goes live, status flips to **LIVE** |

- MH-Z19E readings drift for the first few minutes after a cold start — normal.
- Onboard LED blinking every 5 s = CO2 poll heartbeat (it shares GPIO2).

## 5. Troubleshooting

| Symptom | Fix |
|---|---|
| `could not open port` on upload | close the serial monitor / Arduino IDE; unplug-replug |
| Upload fails at "Connecting..." | lower `upload_speed` to `115200` in platformio.ini; try a different USB cable (must be a *data* cable) |
| Board boots into garbage / bootloops | check D8 (GPIO15) isn't accidentally wired to 3V3, and D3/D4 aren't shorted to GND — these are boot-strap pins (see WIRING.md §5) |
| TFT stays white | CS/DC/RESET wiring; confirm TFT VCC on 3V3 |
| Random resets when sensors attached | weak USB supply — use a ≥1 A adapter/port |
| `NO DATA` for PM | PMS5003 TXD really on D2? fan spinning (faint whir)? |
| `NO DATA` for CO2 | Tx/Rx swapped? (sensor Tx → D1, sensor Rx → D4) |
