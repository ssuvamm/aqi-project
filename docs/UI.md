# UI Guide — what's on the screen and what it means

Two screens. Switch between them with the **push button on D0** or by typing
**`d`** in the serial monitor (see docs/FLASHING.md §3 for how to open it).

---

## Screen 1 — Dashboard

```
┌──────────────────────────────── header ─────────────────────────────────┐
│ AIR QUALITY                                                    LIVE ●   │
├───────────────────────┬──────────────────────────────────────────────────┤
│  US AQI               │  CO2 ppm                                         │
│        127            │        518                                       │
│   ( SENSITIVE )       │     ( FRESH )                    ← colored pills │
├───────────────────────┴──────────────────────────────────────────────────┤
│ ▮▮▮▮▮▮▮▮▮▮▮ AQI gauge ▮▮▮▮▮▮▮▮▮▮▮        ▲ = where you are on the scale  │
├──────────────┬───────────────┬────────────────────────────────────────────┤
│ PM1.0        │ PM2.5         │ PM10                                       │
│   29         │   46          │   61          ← µg/m³, colored (see below) │
└──────────────┴───────────────┴────────────────────────────────────────────┘
```

### Header status (top-right)

| Status | Meaning |
|---|---|
| 🟡 **WARM-UP** | Sensors still stabilizing after power-on (PMS5003 needs ~30 s, MH-Z19E ~60 s). Values shown as `--`. |
| 🟢 **LIVE** | Both sensors delivered a valid, checksum-verified reading in the last 30 s. |
| 🔴 **NO DATA** | One or both sensors silent/garbled for >30 s. Check wiring; `--` shown where data is missing. |

### US AQI card (left)

The **US EPA Air Quality Index** computed from PM2.5 and PM10 (whichever is
worse counts). It uses the EPA's May-2024 revised breakpoints. Note: officially
AQI is a 24-hour average; this device applies the scale to live readings, like
every consumer monitor does — treat it as "AQI right now".

| AQI | Pill | Meaning |
|---|---|---|
| 0–50 | 🟢 GOOD | Air is clean. |
| 51–100 | 🟡 MODERATE | Acceptable; unusually sensitive people may feel effects. |
| 101–150 | 🟠 SENSITIVE | "Unhealthy for Sensitive Groups" — kids, elderly, asthma/heart conditions should reduce prolonged exertion. |
| 151–200 | 🔴 UNHEALTHY | Everyone may begin to feel effects. Ventilate/filter. |
| 201–300 | 🟣 V.UNHEALTHY | Health alert. Avoid exertion, run air purifier. |
| 301+ | 🟤 HAZARDOUS | Emergency conditions. |

### CO2 card (right)

CO2 concentration in ppm from the MH-Z19E. Outdoor air is ~420 ppm; indoor CO2
is a proxy for **how stale the air is / how much exhaled air you're rebreathing**.
High CO2 correlates with drowsiness and poor concentration.

| CO2 | Pill | Meaning |
|---|---|---|
| < 800 ppm | 🟢 FRESH | Well ventilated. |
| 800–1199 | 🟡 OK | Fine, but ventilation is lagging behind occupancy. |
| 1200–1999 | 🟠 STUFFY | Open a window — concentration/drowsiness effects likely. |
| ≥ 2000 | 🔴 BAD | Seriously under-ventilated. |

### AQI gauge bar

Six fixed colored segments = the six AQI categories above (green → maroon).
The white ▲ marker sits at the current AQI's position within its category
segment. No marker while data is warming up or missing.

### PM tiles (bottom)

Mass concentration in **µg/m³** of particles smaller than 1.0, 2.5 and 10 µm
("atmospheric environment" values from the PMS5003).

- **PM2.5** — fine particles that penetrate deep into lungs/bloodstream; the
  main health metric. Value is colored by its own AQI sub-index.
- **PM10** — coarser dust/pollen; also colored by its own sub-index.
- **PM1.0** — ultrafine fraction; no official EPA scale, so always neutral color.

---

## Screen 2 — Details

A raw-data table, updated every 5 s:

| Row | Meaning |
|---|---|
| PM1.0 / PM2.5 / PM10 ug/m3 | Same values as the dashboard tiles. |
| >0.3um … >10um /0.1L | **Particle counts**: number of particles larger than the stated diameter in 0.1 L of air. The >0.3 µm count reacts fastest — watch it spike when you light a match. |
| CO2 ppm | Same as dashboard, color-coded. |
| Sensor temp C | MH-Z19E's **internal die temperature** — a few °C above room temperature, not a room thermometer. |
| AQI PM2.5 / PM10 | The two AQI sub-indices before taking the max (tells you which pollutant is driving the headline number). |
| Uptime | h:mm:ss since power-on. |

---

## Controls & indicators

| Thing | Behavior |
|---|---|
| Push button (D0 ↔ 3V3) | Toggle dashboard/details. |
| `d` in serial monitor | Same toggle. |
| Onboard blue LED | Blinks every 5 s — that's the CO2 UART poll sharing GPIO2. Free heartbeat: blinking = firmware alive. |
| `--` values | Warm-up or no valid data in the last 30 s. |
