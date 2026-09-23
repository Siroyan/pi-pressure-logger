#pragma once
#include <Arduino.h>
#include <esp_timer.h>

// Monotonic milliseconds since boot, independent of NTP and millis() wrap.
inline uint64_t acquisitionMillis() {
  return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
}

struct PressureSample {
  float p0;
  float p1;
  uint64_t timestamp;
};
