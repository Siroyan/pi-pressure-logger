#pragma once
#include <Arduino.h>
#include <Wire.h>

// ADS1015 register layout: TI SBAS473F, sections 7.5.4 and 8.
// All I2C transfers are checked; no library conversion loop can wait forever.
class ADCReader {
  TwoWire& wire;
  bool initialized = false;
  static constexpr uint8_t address = 0x48;
  static constexpr uint32_t pairDeadlineUs = 8000;
  uint32_t started = 0;
  bool expired() const { return static_cast<uint32_t>(micros() - started) >= pairDeadlineUs; }
  bool readRegister(uint8_t reg, uint16_t& value) {
    if (expired()) return false;
    wire.beginTransmission(address);
    const size_t written = wire.write(reg);
    // Always finish the transaction to release Wire's lock, even on a short write.
    const uint8_t result = wire.endTransmission(true);
    if (written != 1 || result != 0 || expired()) return false;
    if (wire.requestFrom(address, static_cast<uint8_t>(2)) != 2 || expired()) return false;
    int high = wire.read(), low = wire.read();
    if (high < 0 || low < 0) return false;
    value = (static_cast<uint16_t>(high) << 8) | low;
    return true;
  }
  bool readChannel(uint8_t channel, int16_t& counts) {
    // Single shot, AINx-GND, +/-6.144V, 3300SPS, comparator disabled.
    const uint16_t config = 0x8000 | ((channel + 4) << 12) | 0x0100 | 0x00C0 | 0x0003;
    if (expired()) return false;
    wire.beginTransmission(address);
    const uint8_t bytes[] = {1, static_cast<uint8_t>(config >> 8), static_cast<uint8_t>(config)};
    const size_t written = wire.write(bytes, sizeof(bytes));
    const uint8_t result = wire.endTransmission(true);
    if (written != sizeof(bytes) || result != 0) return false;
    uint16_t status;
    do {
      if (!readRegister(1, status)) return false;
      if ((status & 0x7FFF) != (config & 0x7FFF)) return false;
      if (status & 0x8000) break;
      delayMicroseconds(100);
    } while (!expired());
    if (!(status & 0x8000)) return false;
    uint16_t raw;
    if (!readRegister(0, raw)) return false;
    // The twelve signed result bits are left-justified in the register.
    if (raw & 0x000F) return false;
    counts = static_cast<int16_t>(raw) / 16;
    return true;
  }
public:
  explicit ADCReader(TwoWire& bus = Wire) : wire(bus) {}
  bool init() {
    // M5.begin() leaves I2C disabled by default. The former ADS library called
    // Wire.begin() internally; the direct register reader must initialize it.
    if (!initialized) initialized = wire.begin(21, 22);
    if (initialized) wire.setTimeOut(2);
    return initialized;
  }
  bool readPair(float& p0, float& p1) {
    // AcquisitionService throttles failed reads (including bus setup) to 1 Hz.
    if (!initialized && !init()) return false;
    started = micros();
    int16_t a, b;
    if (!readChannel(0, a) || !readChannel(1, b)) return false;
    p0 = (a * 0.006f - 1.0f) / 4.0f;
    p1 = (b * 0.006f - 1.0f) / 4.0f;
    return true;
  }
};
