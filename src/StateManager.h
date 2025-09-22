#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <Arduino.h>

// Forward declarations
class SDManager;
class MQTTManager;
class DisplayManager;

extern float ch0_buffer[];
extern float ch1_buffer[];
extern int buf_index;
extern const int buffer_size;

enum SystemState {
  STANDBY,
  RECORDING
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
  
  void setManagers(SDManager* sd, MQTTManager* mqtt, DisplayManager* display);
  SystemState getCurrentState();
  void transitionToStandby();
  void transitionToRecording();
  void toggleState();
  void processSensorData(float v0, float v1, unsigned long now);
  void handleStateSpecificActions(float v0, float v1, unsigned long now);
  
private:
  void onEnterStandby();
  void onEnterRecording();
  void handleStandbyState();
  void handleRecordingState(float v0, float v1, unsigned long now);
};

#endif // STATE_MANAGER_H