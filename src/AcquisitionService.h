#pragma once
#include "ADCReader.h"
#include "RecordingQueue.h"

class AcquisitionService {
  ADCReader reader;
  RecordingQueue& queue;
  std::atomic<bool> available{false};
  bool retryPending = false;
  uint32_t failedAt = 0;
public:
  explicit AcquisitionService(RecordingQueue& output, TwoWire& wire = Wire) : reader(wire), queue(output) {}
  void init() { reader.init(); }
  bool isAvailable() const { return available.load(); }
  // 読み取り失敗時は録画終了を通知し、次の読み取りを1秒後まで待つ。
  void step() {
    if (retryPending && static_cast<uint32_t>(millis() - failedAt) < 1000) return;
    float p0, p1;
    if (!reader.readPair(p0,p1)) {
      available = false;
      failedAt = millis();
      retryPending = true;
      queue.stop();
      return;
    }
    retryPending = false;
    available = true;
    queue.submit(p0,p1);
  }
};
