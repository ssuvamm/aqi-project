// AQI Monitor v1 — ESP8266 D1 Mini + PMS5003 + MH-Z19E + ILI9341 touch TFT
//
// Both sensors are 9600-baud UART devices on SoftwareSerial (the single full
// hardware UART stays on USB for logs/flashing). Bit-banged RX is only
// reliable for one port at a time, so a small state machine takes turns:
//
//   PMS_LISTEN --frame or 3.5s--> CO2_WAIT --reply or 0.5s--> IDLE --5s tick--> ...
//
// The TFT is only redrawn in the hand-off moments (never while a PMS frame is
// being received) so long SPI bursts can't corrupt SoftwareSerial timing.
// Corrupted frames are rejected by checksum and simply retried next cycle.

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>

#include "config.h"
#include "pms5003.h"
#include "mhz19.h"
#include "aqi.h"
#include "ui.h"

SoftwareSerial pmsSerial;
SoftwareSerial mhzSerial;
Pms5003 pms(pmsSerial);
Mhz19 mhz(mhzSerial);

enum class Phase : uint8_t { PMS_LISTEN, CO2_WAIT, IDLE };
Phase phase = Phase::IDLE;
uint32_t phaseStart = 0;
uint32_t cycleStart = 0;

PmsData pmData{};
uint32_t pmLastOkMs = 0;
bool pmEverValid = false;

int co2Ppm = -1;
int co2Temp = 0;
uint32_t co2LastOkMs = 0;
bool co2EverValid = false;

void startCycle() {
    cycleStart = phaseStart = millis();
    mhzSerial.enableRx(false);
    pms.reset();
    pmsSerial.enableRx(true);
    while (pmsSerial.available()) pmsSerial.read();  // drop stale bytes
    phase = Phase::PMS_LISTEN;
}

void startCo2Request() {
    pmsSerial.enableRx(false);
    mhzSerial.enableRx(true);
    while (mhzSerial.available()) mhzSerial.read();
    mhz.requestReading();
    phaseStart = millis();
    phase = Phase::CO2_WAIT;
}

void publishReadings() {
    uint32_t now = millis();
    UiModel m;
    m.pmWarmup  = now < PMS_WARMUP_MS;
    m.co2Warmup = now < MHZ_WARMUP_MS;
    m.pmValid  = pmEverValid && !m.pmWarmup && (now - pmLastOkMs) < DATA_STALE_MS;
    m.co2Valid = co2EverValid && !m.co2Warmup && (now - co2LastOkMs) < DATA_STALE_MS;
    m.uptimeS = now / 1000;

    if (pmEverValid) {
        m.pm1 = pmData.pm1_atm;  m.pm25 = pmData.pm25_atm;  m.pm10 = pmData.pm10_atm;
        m.n03 = pmData.n03;  m.n05 = pmData.n05;  m.n10 = pmData.n10;
        m.n25 = pmData.n25;  m.n50 = pmData.n50;  m.n100 = pmData.n100;
        m.aqiPm25 = aqiFromPm25(pmData.pm25_atm);
        m.aqiPm10 = aqiFromPm10(pmData.pm10_atm);
        m.aqi = max(m.aqiPm25, m.aqiPm10);
    }
    if (co2EverValid) {
        m.co2 = co2Ppm;
        m.temp = co2Temp;
    }
    uiSetModel(m);

    Serial.printf("[%6lus] PM1=%u PM2.5=%u PM10=%u ug/m3 | CO2=%d ppm T=%d C | AQI=%d %s%s\n",
                  (unsigned long)m.uptimeS, m.pm1, m.pm25, m.pm10, m.co2, m.temp, m.aqi,
                  m.pmValid ? "" : "(PM stale/warmup) ",
                  m.co2Valid ? "" : "(CO2 stale/warmup)");
}

void setup() {
    Serial.begin(DEBUG_BAUD);
    delay(50);
    Serial.println("\n\nAQI Monitor v0.1.0 (PMS5003 + MH-Z19E + ILI9341)");

    WiFi.mode(WIFI_OFF);  // no networking in v1 — saves ~70 mA and heat

    pinMode(PIN_BUTTON, INPUT_PULLDOWN_16);  // D0: only pin with pull-DOWN; button feeds 3V3

    pmsSerial.begin(PMS_BAUD, SWSERIAL_8N1, PIN_PMS_RX, -1);   // RX only
    mhzSerial.begin(MHZ_BAUD, SWSERIAL_8N1, PIN_MHZ_RX, PIN_MHZ_TX);
    mhzSerial.enableRx(false);

    uiBegin();

    mhz.setAutoCalibration(MHZ19_ABC_ENABLED);
    Serial.printf("MH-Z19E ABC auto-calibration: %s\n", MHZ19_ABC_ENABLED ? "on" : "off");

    startCycle();
}

void loop() {
    uint32_t now = millis();

    switch (phase) {
        case Phase::PMS_LISTEN:
            if (pms.poll()) {
                pmData = pms.data();
                pmLastOkMs = now;
                pmEverValid = true;
                startCo2Request();
            } else if (now - phaseStart > PMS_LISTEN_TIMEOUT_MS) {
                startCo2Request();   // no frame this cycle — try CO2 anyway
            }
            break;

        case Phase::CO2_WAIT:
            if (mhz.poll()) {
                co2Ppm = mhz.ppm();
                co2Temp = mhz.temperature();
                co2LastOkMs = now;
                co2EverValid = true;
                mhzSerial.enableRx(false);
                publishReadings();
                phase = Phase::IDLE;
            } else if (now - phaseStart > MHZ_RESPONSE_TIMEOUT_MS) {
                mhzSerial.enableRx(false);
                publishReadings();
                phase = Phase::IDLE;
            }
            break;

        case Phase::IDLE:
            if (now - cycleStart >= SAMPLE_INTERVAL_MS) startCycle();
            break;
    }

    // Screen toggle: push button on D0 (pressed = HIGH), or 'd' over the
    // serial monitor. Rising-edge detect + 250 ms lockout as debounce.
    static bool btnPrev = false;
    static uint32_t btnLastMs = 0;
    bool btn = digitalRead(PIN_BUTTON) == HIGH;
    if (btn && !btnPrev && now - btnLastMs > 250) {
        btnLastMs = now;
        uiToggleScreen();
    }
    btnPrev = btn;

    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c == 'd' || c == 'D') uiToggleScreen();
    }
    yield();
}
