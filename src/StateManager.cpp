#include "StateManager.h"
void StateManager::transitionToStandby() {
  if (state==RECORDING && !queue.stop()) return;
  state=STANDBY;
}
void StateManager::transitionToRecording() {
  if (recordingReady && state==STANDBY && queue.start()) state=RECORDING;
}
void StateManager::transitionToFileList() { if (state==STANDBY) state=FILE_LIST; }
void StateManager::toggleState() {
  if (state==STANDBY) transitionToRecording();
  else transitionToStandby();
}
