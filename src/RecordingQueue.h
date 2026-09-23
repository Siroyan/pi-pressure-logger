#pragma once
#include "RecordingSession.h"
#include <atomic>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Only the short FIFO/mailbox operations run under this lock. No peripheral I/O.
class RecordingQueue {
  QueueHandle_t queue = nullptr;
  QueueHandle_t displayQueue = nullptr;
  QueueHandle_t networkQueue = nullptr;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  RecordingSession session;
  uint32_t sessionDrops = 0, sessionHighWater = 0;
  std::atomic<uint32_t> dropped{0}, highWater{0}, currentSession{0};
  bool enqueue(RecordEvent event, unsigned reserve) {
    if (!queue || !displayQueue || !networkQueue) return false;
    if (event.kind == RecordKind::Start) {
      if (uxQueueSpacesAvailable(queue) <= reserve) return false;
      sessionDrops = sessionHighWater = 0;
      currentSession = event.session;
    }
    event.dropped = sessionDrops;
    event.highWater = sessionHighWater;
    bool ok = true;
    if (event.kind != RecordKind::Sample || event.session) {
      ok = uxQueueSpacesAvailable(queue) > reserve && xQueueSend(queue, &event, 0) == pdTRUE;
      if (!ok && event.kind == RecordKind::Sample) { ++sessionDrops; ++dropped; }
      const uint32_t depth = uxQueueMessagesWaiting(queue);
      if (depth > sessionHighWater) sessionHighWater = depth;
      if (depth > highWater) highWater = depth;
    }
    // Independent consumers: SD stalls cannot hold up the screen or cloud.
    if (event.kind == RecordKind::Sample) xQueueSend(displayQueue, &event.sample, 0);
    xQueueOverwrite(networkQueue, &event);
    return ok;
  }
public:
  ~RecordingQueue() {
    if (queue) vQueueDelete(queue);
    if (displayQueue) vQueueDelete(displayQueue);
    if (networkQueue) vQueueDelete(networkQueue);
  }
  bool init(unsigned capacity) {
    queue = xQueueCreate(capacity, sizeof(RecordEvent));
    displayQueue = xQueueCreate(capacity, sizeof(PressureSample));
    networkQueue = xQueueCreate(1, sizeof(RecordEvent));
    return queue && displayQueue && networkQueue;
  }
  bool start() {
    portENTER_CRITICAL(&mux);
    bool ok = session.start(acquisitionMillis(), [this](const RecordEvent& e, unsigned n) { return enqueue(e,n); });
    portEXIT_CRITICAL(&mux);
    return ok;
  }
  bool stop() {
    portENTER_CRITICAL(&mux);
    bool ok = session.stop(acquisitionMillis(), [this](const RecordEvent& e, unsigned n) { return enqueue(e,n); });
    portEXIT_CRITICAL(&mux);
    return ok;
  }
  bool submit(float p0, float p1) {
    portENTER_CRITICAL(&mux);
    bool ok = session.sample({p0,p1,acquisitionMillis()}, [this](const RecordEvent& e, unsigned n) { return enqueue(e,n); });
    portEXIT_CRITICAL(&mux);
    return ok;
  }
  bool receive(RecordEvent& event) { return queue && xQueueReceive(queue, &event, 0) == pdTRUE; }
  bool receiveDisplay(PressureSample& sample) { return displayQueue && xQueueReceive(displayQueue, &sample, 0) == pdTRUE; }
  bool receiveNetwork(RecordEvent& event) { return networkQueue && xQueueReceive(networkQueue, &event, 0) == pdTRUE; }
  bool empty() const { return !queue || uxQueueMessagesWaiting(queue) == 0; }
  uint32_t droppedSamples() const { return dropped.load(); }
  uint32_t maxDepth() const { return highWater.load(); }
  uint32_t sessionId() const { return currentSession.load(); }
};
