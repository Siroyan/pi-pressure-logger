#pragma once
#include "RecordingQueue.h"

enum SystemState { STANDBY, RECORDING, FILE_LIST };

// UI-owned state. Peripheral work is represented by ordered queue boundaries.
class StateManager {
  RecordingQueue& queue;
  SystemState state=STANDBY;
  bool recordingReady=false;
public:
  explicit StateManager(RecordingQueue& output) : queue(output) {}
  void setRecordingReady(bool ready) { recordingReady=ready; }
  SystemState getCurrentState() const { return state; }
  void transitionToStandby();
  void transitionToRecording();
  void transitionToFileList();
  void toggleState();
};
