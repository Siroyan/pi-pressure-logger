#pragma once
#include "RecordingQueue.h"

enum SystemState { STANDBY, RECORDING, FILE_LIST };

// 画面側が管理する状態。周辺機器への処理は順序付きのキュー通知で伝える。
class StateManager {
  RecordingQueue& queue;
  SystemState state=STANDBY;
  bool recordingReady=false;
public:
  explicit StateManager(RecordingQueue& output) : queue(output) {}
  void setRecordingReady(bool ready) { recordingReady=ready; }
  SystemState getCurrentState() const { return state; }
  // 終了通知をキューへ入れられない場合は録画状態を維持する。
  void transitionToStandby();
  // 計測タスクの準備と開始通知の受付が揃った場合だけ録画状態へ移る。
  void transitionToRecording();
  void transitionToFileList();
  void toggleState();
};
