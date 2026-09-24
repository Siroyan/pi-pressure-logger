#pragma once
#include <Arduino.h>
#include <Wire.h>

// ADS1015のレジスタ配置はTIの資料SBAS473Fの7.5.4節と8節に従う。
// I²C通信の結果を毎回確認し、変換待ちが無期限に続かないようにする。
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
    // 書き込みが短く終わっても、通信を終了してWireのロックを解放する。
    const uint8_t result = wire.endTransmission(true);
    if (written != 1 || result != 0 || expired()) return false;
    if (wire.requestFrom(address, static_cast<uint8_t>(2)) != 2 || expired()) return false;
    int high = wire.read(), low = wire.read();
    if (high < 0 || low < 0) return false;
    value = (static_cast<uint16_t>(high) << 8) | low;
    return true;
  }
  bool readChannel(uint8_t channel, int16_t& counts) {
    // 単発変換、AINx-GND、±6.144 V、3300 SPS、比較器は無効。
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
    // 符号付き12ビットの変換結果はレジスタ内で左詰めされる。
    if (raw & 0x000F) return false;
    counts = static_cast<int16_t>(raw) / 16;
    return true;
  }
public:
  explicit ADCReader(TwoWire& bus = Wire) : wire(bus) {}
  bool init() {
    // M5.begin()後もI²Cは未初期化。以前のADSライブラリは内部で
    // Wire.begin()を呼んでいたため、直接読む場合はここで初期化する。
    if (!initialized) initialized = wire.begin(21, 22);
    if (initialized) wire.setTimeOut(2);
    return initialized;
  }
  // 2チャンネルを1組として読み、両方成功した場合だけMPa単位の出力を書き換える。
  bool readPair(float& p0, float& p1) {
    // 通信初期化を含む読み取り失敗の再試行はAcquisitionServiceが1秒間隔に抑える。
    if (!initialized && !init()) return false;
    started = micros();
    int16_t a, b;
    if (!readChannel(0, a) || !readChannel(1, b)) return false;
    p0 = (a * 0.006f - 1.0f) / 4.0f;
    p1 = (b * 0.006f - 1.0f) / 4.0f;
    return true;
  }
};
