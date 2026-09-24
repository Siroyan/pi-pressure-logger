#pragma once
#include <cstdint>
#include <mutex>
using BaseType_t = int;
using UBaseType_t = unsigned;
using TickType_t = uint32_t;
constexpr BaseType_t pdTRUE=1, pdFALSE=0;
constexpr TickType_t portMAX_DELAY=UINT32_MAX;
using portMUX_TYPE = std::mutex;
#define portMUX_INITIALIZER_UNLOCKED {}
inline void portENTER_CRITICAL(portMUX_TYPE* mutex) { mutex->lock(); }
inline void portEXIT_CRITICAL(portMUX_TYPE* mutex) { mutex->unlock(); }
