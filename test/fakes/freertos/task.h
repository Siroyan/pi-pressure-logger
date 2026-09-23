#pragma once
#include "FreeRTOS.h"
#include "Arduino.h"
#include <string>
#include <vector>
constexpr unsigned tskIDLE_PRIORITY=0;
constexpr BaseType_t pdPASS=1;
inline unsigned task_calls=0, task_fail_at=0;
inline std::vector<std::string> task_names;
inline BaseType_t xTaskCreatePinnedToCore(void (*)(void*),const char* name,unsigned,void*,unsigned,void*,int) {
  ++task_calls;
  if (task_calls==task_fail_at) return pdFALSE;
  task_names.emplace_back(name); return pdPASS;
}
inline TickType_t xTaskGetTickCount() { return fake_millis; }
inline TickType_t pdMS_TO_TICKS(uint32_t ms) { return ms; }
inline void vTaskDelay(TickType_t ticks) { fake_millis+=ticks; }
inline void vTaskDelayUntil(TickType_t* last,TickType_t ticks) { *last+=ticks; fake_millis=*last; }
