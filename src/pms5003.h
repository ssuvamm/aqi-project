#pragma once
#include <Arduino.h>

// Plantower PMS5003 driver (active mode, read-only).
// Frame format per datasheet Appendix I: 0x42 0x4D + length + 13 data words
// + 16-bit additive checksum, 32 bytes total, 9600 8N1.

struct PmsData {
    // "CF=1" standard-particle values (factory calibration environment)
    uint16_t pm1_std, pm25_std, pm10_std;
    // "under atmospheric environment" values — use these for AQI
    uint16_t pm1_atm, pm25_atm, pm10_atm;
    // particle counts per 0.1 L of air, by minimum diameter
    uint16_t n03, n05, n10, n25, n50, n100;
};

class Pms5003 {
public:
    explicit Pms5003(Stream& serial) : _ser(serial) {}

    // Discard any half-received frame (call when re-enabling RX).
    void reset() { _idx = 0; }

    // Feed available bytes; returns true when a complete valid frame was
    // parsed (data() is then updated). Call repeatedly from loop().
    bool poll();

    const PmsData& data() const { return _data; }

private:
    Stream& _ser;
    uint8_t _buf[32];
    uint8_t _idx = 0;
    PmsData _data{};
};
