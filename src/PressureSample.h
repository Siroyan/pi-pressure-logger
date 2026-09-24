#pragma once
#include <Arduino.h>
#include <esp_timer.h>

// 起動後の単調増加するミリ秒。時刻同期やmillis()の周回に影響されない。
inline uint64_t acquisitionMillis() {
  return static_cast<uint64_t>(esp_timer_get_time()) / 1000;
}

// 圧力はMPa、timestampはacquisitionMillis()による起動後ミリ秒。
struct PressureSample {
  float p0;
  float p1;
  uint64_t timestamp;
};
