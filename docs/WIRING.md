# Wiring Guide — AQI Monitor v1

Hardware:
- **ESP8266 D1 Mini V2** (NodeMCU-style, CH340 USB, 3.3 V logic, onboard 5V→3.3V regulator)
- **Plantower PMS5003** PM1.0/PM2.5/PM10 laser particle sensor
- **Winsen MH-Z19E** NDIR CO2 sensor
- **2.4" SPI TFT 240×320** (ILI9341 driver + XPT2046 resistive touch, robu.in module)

> ⚠️ **Wire everything with USB disconnected.** Double-check 5V vs 3V3 rails before
> powering up — the TFT and the ESP8266 are 3.3 V parts, the two sensors need 5 V
> *power* but speak 3.3 V on their data lines (confirmed in both datasheets), so no
> level shifters are needed anywhere.

---

## 1. Power rails

| Rail | Who is on it | Current |
|---|---|---|
| **5V** (USB) | PMS5003 VCC, MH-Z19E Vin | ≤100 mA + ≤125 mA peak |
| **3V3** (onboard regulator) | TFT VCC, TFT LED (backlight) | ~60–80 mA |
| **G** | everything | — |

- All grounds must be common: PMS5003 pin 2, MH-Z19E GND, TFT GND → D1 Mini **G**.
- Use a **good USB power source (≥1 A)**. Peak draw of the whole stack is ~400 mA;
  weak phone-charger cables cause brownouts that look like random resets.
- MH-Z19E datasheet requires 5.0 V ±0.1 V — powering it from a long thin USB cable
  can sag below that. If CO2 readings look stuck/low, measure the 5V pin.

## 2. PMS5003 (8-pin 1.27 mm connector — comes with a pigtail cable)

Pin numbering from the datasheet (Figure 2, "Pin Definition"), pin 1 = VCC:

| PMS5003 pin | Name | D1 Mini | GPIO | Why |
|---|---|---|---|---|
| 1 | VCC | **5V** | — | fan needs 5 V |
| 2 | GND | **G** | — | |
| 3 | SET | — (NC) | — | internally pulled up = "running". Reserved for future sleep control |
| 4 | RXD | — (NC) | — | v1 never sends commands (active mode is the power-on default) |
| 5 | TXD | **D2** | GPIO4 | sensor streams a 32-byte frame every ~2.3 s at 9600 baud |
| 6 | RESET | — (NC) | — | internally pulled up |
| 7, 8 | NC | — | — | datasheet: "should not be connected" |

Notes:
- The metal shell is internally tied to GND — don't let it touch live pins.
- Mount so the fan inlet/outlet aren't blocked; stable data ~30 s after power-up.

## 3. MH-Z19E (pin names printed on the sensor PCB)

| MH-Z19E pin | D1 Mini | GPIO | Why |
|---|---|---|---|
| Vin | **5V** | — | 5.0 V ±0.1 V, peak 125 mA |
| GND | **G** | — | |
| Tx (UART out) | **D1** | GPIO5 | sensor's answer → ESP receive |
| Rx (UART in) | **D4** | GPIO2 | ESP transmit → sensor. GPIO2 idles HIGH = safe boot-strap state |
| PWM | — (NC) | — | UART is used instead |
| HD | — (NC) | — | pulling low >7 s force-calibrates zero — leave alone |

Notes:
- **Tx/Rx cross over** (sensor Tx → ESP RX pin, sensor Rx → ESP TX pin).
- D4 also drives the D1 Mini's onboard LED — it blinks briefly at every CO2 poll.
  That's normal and doubles as a heartbeat.
- Preheat time is 1 min; readings before that are ignored by the firmware.
- Keep it away from the TFT/ESP heat and away from your own breath while testing.

## 4. 2.4" ILI9341 TFT + XPT2046 touch (14-pin header row)

The display and the touch controller share one SPI bus (SCK/MOSI), each with its
own chip-select. **Two D1 Mini pins each feed two TFT pins** — use breadboard rows
or Y-jumpers.

| TFT pin | D1 Mini | GPIO | Note |
|---|---|---|---|
| VCC | **3V3** | — | |
| GND | **G** | — | |
| CS | **D8** | GPIO15 | display chip-select. D1 Mini's onboard pull-down keeps boot safe |
| RESET | **RST** | — | display resets together with the ESP (`TFT_RST=-1` in platformio.ini) |
| DC | **D3** | GPIO0 | data/command. Onboard pull-up keeps boot-strap safe |
| SDI (MOSI) | **D7** | GPIO13 | shared with T_DIN |
| SCK | **D5** | GPIO14 | shared with T_CLK |
| LED | **3V3** | — | backlight, always on in v1 |
| SDO (MISO) | **— (NC)** | — | deliberately NOT wired: ILI9341's SDO doesn't tri-state cleanly and would corrupt touch reads |
| T_CLK | **D5** | GPIO14 | same wire as SCK |
| T_CS | **D0** | GPIO16 | touch chip-select |
| T_DIN | **D7** | GPIO13 | same wire as SDI |
| T_DO | **D6** | GPIO12 | touch data out — the only MISO device on the bus |
| T_IRQ | — (NC) | — | firmware polls pressure instead |
| SD_CS / SD_MOSI / SD_MISO / SD_SCK | — (NC) | — | microSD slot unused in v1 |

Notes:
- Keep SPI wires short (≤15 cm); the bus runs at 27 MHz.
- If the panel stays white: re-check CS/DC/RESET. If colors are inverted or the
  image is mirrored, tell Claude — it's a one-line driver flag.

## 5. Complete D1 Mini pin budget

| D1 Mini | GPIO | Used for | Boot-strap constraint |
|---|---|---|---|
| TX | GPIO1 | USB serial log (115200) | keep free |
| RX | GPIO3 | USB serial / flashing | keep free |
| D0 | GPIO16 | Touch T_CS | none (no interrupt pin — fine for CS) |
| D1 | GPIO5 | MH-Z19E Tx → ESP | none |
| D2 | GPIO4 | PMS5003 TXD → ESP | none |
| D3 | GPIO0 | TFT DC | must be HIGH at boot — onboard pull-up, DC is an input on the TFT, safe |
| D4 | GPIO2 | → MH-Z19E Rx | must be HIGH at boot — UART idle is HIGH, safe |
| D5 | GPIO14 | SPI SCK (TFT + touch) | none |
| D6 | GPIO12 | SPI MISO (touch T_DO only) | none |
| D7 | GPIO13 | SPI MOSI (TFT + touch) | none |
| D8 | GPIO15 | TFT CS | must be LOW at boot — onboard pull-down, safe |
| A0 | ADC0 | free | reserved for future use |
| 5V | — | PMS5003 + MH-Z19E power | |
| 3V3 | — | TFT power + backlight | |
| RST | — | TFT RESET | |
| G | — | common ground | |

Every GPIO is now allocated. Future expansions (more I2C sensors like BME280)
would share D1/D2 as an I2C bus — that requires moving both UART sensors around,
so plan with Claude first.

## ASCII overview

```
                        ┌──────────────────────┐
        PMS5003         │    ESP8266 D1 Mini   │          2.4" ILI9341 TFT
     ┌───────────┐      │                      │         ┌───────────────┐
     │ 1 VCC ────┼──────┤ 5V              3V3  ├─────────┤ VCC           │
     │ 2 GND ────┼──────┤ G               3V3  ├─────────┤ LED           │
     │ 5 TXD ────┼──────┤ D2              G    ├─────────┤ GND           │
     └───────────┘      │                 D8   ├─────────┤ CS            │
                        │                 RST  ├─────────┤ RESET         │
        MH-Z19E         │                 D3   ├─────────┤ DC            │
     ┌───────────┐      │                 D7   ├────┬────┤ SDI (MOSI)    │
     │ Vin ──────┼──────┤ 5V              D5   ├─┬──┼────┤ SCK           │
     │ GND ──────┼──────┤ G               │    │ │  │    │ SDO   (NC!)   │
     │ Tx  ──────┼──────┤ D1              │    │ └──┼────┤ T_CLK         │
     │ Rx  ──────┼──────┤ D4              D0   ├────┼────┤ T_CS          │
     └───────────┘      │                 │    │    └────┤ T_DIN         │
                        │                 D6   ├─────────┤ T_DO          │
                        └──────────────────────┘         │ T_IRQ (NC)    │
                                                         └───────────────┘
```
