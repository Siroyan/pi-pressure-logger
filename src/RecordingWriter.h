#pragma once
#include "RecordingSession.h"
#include "SDManager.h"

// Consumed in FIFO order, independent of which UI screen is currently shown.
class RecordingWriter {
  SDManager& sd;
  uint32_t activeSession = 0;
public:
  explicit RecordingWriter(SDManager& storage) : sd(storage) {}
  bool process(const RecordEvent& event) {
    if (event.kind == RecordKind::Start) {
      sd.stopRecording();
      activeSession = event.session;
      sd.startRecording(event.sample.timestamp);
    } else if (event.kind == RecordKind::Stop && event.session == activeSession) {
      sd.writeSummary(event.session, event.dropped, event.highWater);
      sd.stopRecording();
      activeSession = 0;
    } else if (event.kind == RecordKind::Sample && activeSession && event.session == activeSession) {
      sd.logData(event.sample);
      return true;  // The network session stays active even after an SD failure.
    }
    return false;
  }
};
