#pragma once
#include "FreeRTOS.h"
#include <deque>
#include <vector>
#include <cstring>
struct FakeQueue { size_t capacity, itemSize; std::deque<std::vector<uint8_t>> items; std::mutex mutex; };
using QueueHandle_t = FakeQueue*;
inline int queue_fail_after=-1;
inline QueueHandle_t xQueueCreate(size_t count,size_t size) {
  if (queue_fail_after==0) return nullptr;
  if (queue_fail_after>0) --queue_fail_after;
  return new FakeQueue{count,size,{}, {}};
}
inline void vQueueDelete(QueueHandle_t queue) { delete queue; }
inline unsigned uxQueueSpacesAvailable(QueueHandle_t q) { std::lock_guard<std::mutex> lock(q->mutex); return q->capacity-q->items.size(); }
inline unsigned uxQueueMessagesWaiting(QueueHandle_t q) { std::lock_guard<std::mutex> lock(q->mutex); return q->items.size(); }
inline BaseType_t xQueueSend(QueueHandle_t q,const void* item,TickType_t) {
  std::lock_guard<std::mutex> lock(q->mutex);
  if(q->items.size()==q->capacity) return pdFALSE;
  const auto* b=static_cast<const uint8_t*>(item); q->items.emplace_back(b,b+q->itemSize); return pdTRUE;
}
inline BaseType_t xQueueReceive(QueueHandle_t q,void* item,TickType_t) {
  std::lock_guard<std::mutex> lock(q->mutex);
  if(q->items.empty()) return pdFALSE;
  memcpy(item,q->items.front().data(),q->itemSize); q->items.pop_front(); return pdTRUE;
}
inline BaseType_t xQueueOverwrite(QueueHandle_t q,const void* item) {
  std::lock_guard<std::mutex> lock(q->mutex);
  q->items.clear(); const auto* b=static_cast<const uint8_t*>(item); q->items.emplace_back(b,b+q->itemSize); return pdTRUE;
}
