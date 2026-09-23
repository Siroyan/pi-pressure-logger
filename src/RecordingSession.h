#pragma once
#include "PressureSample.h"

enum class RecordKind : uint8_t { Start, Sample, Stop };
struct RecordEvent {
  RecordEvent(RecordKind k=RecordKind::Sample, PressureSample s={}, uint32_t id=0, uint32_t seq=0)
      : kind(k), sample(s), session(id), sequence(seq) {}
  RecordKind kind;
  PressureSample sample;
  uint32_t session;
  uint32_t sequence;
};

// Calls are serialized by the queue adapter. The sink reserves capacity for
// boundaries, so a full sample queue cannot lose the closing Stop event.
class RecordingSession {
  uint32_t nextSession = 0;
  uint32_t activeSession = 0;
  uint32_t sequence = 0;
public:
  template<class Sink> bool start(uint64_t now, Sink&& sink) {
    if (activeSession) return false;
    uint32_t id = nextSession + 1;
    if (!id) ++id;
    if (!sink(RecordEvent{RecordKind::Start, {0, 0, now}, id, 0}, 1)) return false;
    nextSession = activeSession = id;
    sequence = 0;
    return true;
  }
  template<class Sink> bool stop(uint64_t now, Sink&& sink) {
    if (!activeSession) return true;
    if (!sink(RecordEvent{RecordKind::Stop, {0, 0, now}, activeSession, sequence}, 0)) return false;
    activeSession = 0;
    return true;
  }
  template<class Sink> bool sample(const PressureSample& sample, Sink&& sink) {
    return sink(RecordEvent{RecordKind::Sample, sample, activeSession, activeSession ? ++sequence : 0}, 2);
  }
};
