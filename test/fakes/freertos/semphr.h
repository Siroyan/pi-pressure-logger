#pragma once
#include "FreeRTOS.h"
#include <thread>
struct FakeSemaphore { std::recursive_mutex mutex; bool recursive; unsigned depth=0; std::thread::id owner; };
using SemaphoreHandle_t = FakeSemaphore*;
inline bool semaphore_fail=false;
inline int semaphore_fail_after=-1;
inline bool fakeSemaphoreAvailable() {
  if (semaphore_fail || semaphore_fail_after==0) return false;
  if (semaphore_fail_after>0) --semaphore_fail_after;
  return true;
}
inline SemaphoreHandle_t xSemaphoreCreateMutex() { if(!fakeSemaphoreAvailable()) return nullptr; return new FakeSemaphore{{},false,0,{}}; }
inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { if(!fakeSemaphoreAvailable()) return nullptr; return new FakeSemaphore{{},true,0,{}}; }
inline void vSemaphoreDelete(SemaphoreHandle_t s) { delete s; }
inline BaseType_t xSemaphoreTake(SemaphoreHandle_t s,TickType_t wait) {
  if (!s->mutex.try_lock()) {
    if (!wait) return pdFALSE;
    s->mutex.lock();
  }
  if (!s->recursive && s->depth && s->owner==std::this_thread::get_id()) {
    s->mutex.unlock(); return pdFALSE;
  }
  ++s->depth; s->owner=std::this_thread::get_id(); return pdTRUE;
}
inline void xSemaphoreGive(SemaphoreHandle_t s) { --s->depth; s->mutex.unlock(); }
inline BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t s,TickType_t t) { return xSemaphoreTake(s,t); }
inline void xSemaphoreGiveRecursive(SemaphoreHandle_t s) { xSemaphoreGive(s); }
