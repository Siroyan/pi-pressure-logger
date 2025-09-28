#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

// Forward declarations
class SDManager;
class MQTTManager;
class DisplayManager;
class TimeManager;

extern float ch0_buffer[];
extern float ch1_buffer[];
extern int buf_index;
extern const int buffer_size;

enum SystemState {
  STANDBY,
  RECORDING,
  FILE_LIST
};

class StateManager {
private:
  SystemState currentState;
  unsigned long stateChangeTime;
  SDManager* sdManager;
  MQTTManager* mqttManager;
  DisplayManager* displayManager;
  
public:
  StateManager();
  
  void setManagers(SDManager* sd, MQTTManager* mqtt, DisplayManager* display, TimeManager* time = nullptr);
  SystemState getCurrentState();
  void transitionToStandby();
  void transitionToRecording();
  void transitionToFileList();
  void toggleState();
  void handleButtonB();
  void processSensorData(float p0, float p1, unsigned long now);
  void handleStateSpecificActions(float p0, float p1, unsigned long now);
  
private:
  void onEnterStandby();
  void onEnterRecording();
  void onEnterFileList();
  void handleStandbyState();
  void handleRecordingState(float p0, float p1, unsigned long now);
  void handleFileListState();
};

#endif // STATE_MANAGER_H