#include "mhz19.h"

namespace {
constexpr uint8_t CMD_READ_CO2  = 0x86;
constexpr uint8_t CMD_ABC       = 0x79;
constexpr uint8_t CMD_ZERO_CAL  = 0x87;

// Winsen checksum: over bytes 1..7, negated + 1.
uint8_t checksum(const uint8_t* frame) {
    uint8_t sum = 0;
    for (uint8_t i = 1; i < 8; i++) sum += frame[i];
    return (uint8_t)(0xFF - sum + 1);
}
}

void Mhz19::sendCommand(uint8_t cmd, uint8_t d0) {
    uint8_t frame[9] = {0xFF, 0x01, cmd, d0, 0, 0, 0, 0, 0};
    frame[8] = checksum(frame);
    _ser.write(frame, sizeof(frame));
}

void Mhz19::requestReading() {
    _idx = 0;
    sendCommand(CMD_READ_CO2);
}

void Mhz19::setAutoCalibration(bool enabled) {
    sendCommand(CMD_ABC, enabled ? 0xA0 : 0x00);
}

void Mhz19::calibrateZero() {
    sendCommand(CMD_ZERO_CAL);
}

bool Mhz19::poll() {
    bool gotFrame = false;
    while (_ser.available()) {
        uint8_t b = (uint8_t)_ser.read();

        if (_idx == 0 && b != 0xFF) continue;
        if (_idx == 1 && b != CMD_READ_CO2) { _idx = (b == 0xFF) ? 1 : 0; continue; }

        _buf[_idx++] = b;
        if (_idx < 9) continue;
        _idx = 0;

        if (checksum(_buf) != _buf[8]) continue;

        _ppm  = (int)_buf[2] << 8 | _buf[3];
        _temp = (int)_buf[4] - 40;
        gotFrame = true;
    }
    return gotFrame;
}
