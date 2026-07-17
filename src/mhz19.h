#pragma once
#include <Arduino.h>

// Winsen MH-Z19E NDIR CO2 sensor driver (UART mode, 9600 8N1).
// Uses the standard Winsen 9-byte command frame:
//   0xFF 0x01 CMD D0 D1 D2 D3 D4 CHECKSUM
// Read concentration = 0x86, ABC on/off = 0x79, zero-point calibrate = 0x87.
// (The MH-Z19E user's manual v1.0 omits the command table; this is the
// common protocol shared by the whole MH-Z19 family.)

class Mhz19 {
public:
    explicit Mhz19(Stream& serial) : _ser(serial) {}

    // Send the "read CO2" command. Answer arrives within ~50 ms; collect it
    // with poll().
    void requestReading();

    // Feed available bytes; returns true when a valid response was parsed
    // (ppm()/temperature() then updated).
    bool poll();

    // ABC = Automatic Baseline Correction (24 h self-calibration to 400 ppm).
    void setAutoCalibration(bool enabled);

    // Zero-point calibration: only call after >=20 min in fresh outdoor air
    // (sensor then treats current reading as 400 ppm). Unused in normal
    // operation; kept for a future settings screen.
    void calibrateZero();

    int ppm() const { return _ppm; }
    int temperature() const { return _temp; }  // rough die temp, +/- a few C

private:
    void sendCommand(uint8_t cmd, uint8_t d0 = 0);

    Stream& _ser;
    uint8_t _buf[9];
    uint8_t _idx = 0;
    int _ppm = -1;
    int _temp = 0;
};
