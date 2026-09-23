#pragma once
#include "RecordingSession.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class RecordingQueue {
  QueueHandle_t queue = nullptr;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  RecordingSession session;
  bool enqueue(const RecordEvent& event, unsigned reserve) {
    return queue && uxQueueSpacesAvailable(queue) > reserve &&
        xQueueSend(queue, &event, 0) == pdTRUE;
  }
public:
  ~RecordingQueue() { if (queue) vQueueDelete(queue); }
  bool init(unsigned capacity) { queue = xQueueCreate(capacity, sizeof(RecordEvent)); return queue; }
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
};
