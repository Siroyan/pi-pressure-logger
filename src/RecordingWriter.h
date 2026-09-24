#pragma once
#include "RecordingSession.h"
#include "SDManager.h"

// 表示中の画面に関係なく、記録イベントを到着順に処理する。
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
      return true;  // SDへの書き込み失敗後も通信セッションは継続する。
    }
    return false;
  }
};
