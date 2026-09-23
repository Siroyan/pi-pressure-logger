#pragma once
#include <cstdint>
inline uint64_t fake_acquisition_ms = 0;
inline int64_t esp_timer_get_time() { return fake_acquisition_ms * 1000; }
