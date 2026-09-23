#include "StateManager.h"
#include "SDManager.h"
#include "MQTTManager.h"
#include "DisplayManager.h"

StateManager::StateManager() : currentState(STANDBY), stateChangeTime(0), 
  sdManager(nullptr), mqttManager(nullptr), displayManager(nullptr) {}

void StateManager::setManagers(SDManager* sd, MQTTManager* mqtt, DisplayManager* display, TimeManager* time) {
  sdManager = sd;
  mqttManager = mqtt;
  displayManager = display;
  
  // Connect TimeManager to SDManager if both are available
  if (time && sdManager) {
    sdManager->setTimeManager(time);
  }
}

SystemState StateManager::getCurrentState() {
  return currentState;
}

void StateManager::transitionToStandby() {
  if (currentState != STANDBY) {
    if (currentState == RECORDING && (!recordingQueue || !recordingQueue->stop())) return;
    currentState = STANDBY;
    stateChangeTime = millis();
    onEnterStandby();
  }
}

void StateManager::transitionToRecording() {
  if (recordingReady && currentState == STANDBY) {
    if (!recordingQueue || !recordingQueue->start()) return;
    currentState = RECORDING;
    stateChangeTime = millis();
    onEnterRecording();
  }
}

void StateManager::transitionToFileList() {
  if (currentState == STANDBY) {
    currentState = FILE_LIST;
    stateChangeTime = millis();
    onEnterFileList();
  }
}

void StateManager::toggleState() {
  if (currentState == STANDBY) {
    transitionToRecording();
  } else if (currentState == RECORDING) {
    transitionToStandby();
  } else if (currentState == FILE_LIST) {
    transitionToStandby();
  }
}

void StateManager::handleButtonB() {
  if (currentState == STANDBY) {
    transitionToFileList();
  } else if (currentState == FILE_LIST) {
    transitionToStandby();
  }
  // Button B does nothing in RECORDING state to prevent accidental interruption
}

void StateManager::onEnterStandby() {
  
  // If coming from FILE_LIST, restore the waveform display
  if (displayManager) {
    displayManager->init();
  }
  
  Serial.println("State: STANDBY");
}

void StateManager::onEnterRecording() {
  Serial.println("State: RECORDING");
}

void StateManager::onEnterFileList() {
  Serial.println("State: FILE_LIST");
  // Display will be updated by the DisplayManager
}

void StateManager::processSensorData(float p0, float p1, uint64_t now) {
  // Always update display buffers
  ch0_buffer[buf_index] = p0;
  ch1_buffer[buf_index] = p1;
  
  // Only update display if not in file list mode
  if (displayManager && currentState != FILE_LIST) {
    displayManager->drawSample(p0, p1, now);
    displayManager->drawPressureText(p0, p1);
  }
  
  // Handle state-specific actions
  // Recording is consumed separately from the current screen state.
  
  buf_index = (buf_index + 1) % buffer_size;
}
