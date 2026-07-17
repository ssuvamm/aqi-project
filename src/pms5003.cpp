#include "pms5003.h"

namespace {
constexpr uint8_t START1 = 0x42;
constexpr uint8_t START2 = 0x4D;
constexpr uint8_t FRAME_LEN = 32;
constexpr uint16_t PAYLOAD_LEN = 28;  // 2x13 data + 2 check bytes

uint16_t word16(const uint8_t* p) { return (uint16_t)p[0] << 8 | p[1]; }
}

bool Pms5003::poll() {
    bool gotFrame = false;
    while (_ser.available()) {
        uint8_t b = (uint8_t)_ser.read();

        if (_idx == 0 && b != START1) continue;
        if (_idx == 1 && b != START2) { _idx = (b == START1) ? 1 : 0; continue; }

        _buf[_idx++] = b;
        if (_idx < FRAME_LEN) continue;
        _idx = 0;

        // Length field must match the fixed 32-byte frame.
        if (word16(_buf + 2) != PAYLOAD_LEN) continue;

        // Checksum = sum of all bytes before the 2 check bytes.
        uint16_t sum = 0;
        for (uint8_t i = 0; i < FRAME_LEN - 2; i++) sum += _buf[i];
        if (sum != word16(_buf + 30)) continue;

        _data.pm1_std  = word16(_buf + 4);
        _data.pm25_std = word16(_buf + 6);
        _data.pm10_std = word16(_buf + 8);
        _data.pm1_atm  = word16(_buf + 10);
        _data.pm25_atm = word16(_buf + 12);
        _data.pm10_atm = word16(_buf + 14);
        _data.n03  = word16(_buf + 16);
        _data.n05  = word16(_buf + 18);
        _data.n10  = word16(_buf + 20);
        _data.n25  = word16(_buf + 22);
        _data.n50  = word16(_buf + 24);
        _data.n100 = word16(_buf + 26);
        gotFrame = true;
    }
    return gotFrame;
}
