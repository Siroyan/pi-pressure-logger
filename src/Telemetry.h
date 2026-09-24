#pragma once
#include <Arduino.h>
#include <esp_timer.h>

// 圧力はMPa、timestampは時刻同期に依存しない起動後ミリ秒。
struct Telemetry {
  float p0=0, p1=0;
  uint64_t timestamp=0;
  uint32_t session=0, sequence=0;
  Telemetry(float a=0, float b=0, uint64_t time=0, uint32_t id=0, uint32_t seq=0)
      : p0(a), p1(b), timestamp(time), session(id), sequence(seq) {}
};

inline String telemetryPayload(const Telemetry& value, const char* device) {
  return "{\"timestamp\":" + String(value.timestamp) + ",\"device\":\"" + String(device) +
      "\",\"session\":" + String(value.session) + ",\"sequence\":" + String(value.sequence) +
      ",\"ch0\":" + String(value.p0,4) + ",\"ch1\":" + String(value.p1,4) + "}";
}
