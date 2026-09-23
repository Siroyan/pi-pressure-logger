#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>
#include "RecordingQueue.h"

// Forward declarations
class SDManager;
class MQTTManager;
class DisplayManager;
class TimeManager;

extern float ch0_buffer[];
extern float ch1_buffer[];
extern int buf_index;
extern const int buffer_size;
extern const int graph_gap_samples;

enum SystemState {
  STANDBY,
  RECORDING,
  FILE_LIST
};

class StateManager {
private:
  SystemState currentState;
  bool recordingReady = false;
  unsigned long stateChangeTime;
  SDManager* sdManager;
  MQTTManager* mqttManager;
  DisplayManager* displayManager;
  RecordingQueue* recordingQueue = nullptr;
  
public:
  StateManager();
  
  void setManagers(SDManager* sd, MQTTManager* mqtt, DisplayManager* display, TimeManager* time = nullptr);
  void setRecordingReady(bool ready) { recordingReady = ready; }
  void setRecordingQueue(RecordingQueue* queue) { recordingQueue = queue; }
  SystemState getCurrentState();
  void transitionToStandby();
  void transitionToRecording();
  void transitionToFileList();
  void toggleState();
  void handleButtonB();
  void processSensorData(float p0, float p1, uint64_t now);
  
private:
  void onEnterStandby();
  void onEnterRecording();
  void onEnterFileList();
};

#endif // STATE_MANAGER_H
