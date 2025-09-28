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
    currentState = STANDBY;
    stateChangeTime = millis();
    onEnterStandby();
  }
}

void StateManager::transitionToRecording() {
  if (currentState != RECORDING) {
    currentState = RECORDING;
    stateChangeTime = millis();
    onEnterRecording();
  }
}

void StateManager::transitionToFileList() {
  if (currentState != FILE_LIST) {
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
  if (sdManager && sdManager->isRecording()) {
    sdManager->stopRecording();
  }
  
  // If coming from FILE_LIST, restore the waveform display
  if (displayManager) {
    displayManager->init();
  }
  
  Serial.println("State: STANDBY");
}

void StateManager::onEnterRecording() {
  if (sdManager && sdManager->isAvailable()) {
    sdManager->startRecording();
  }
  Serial.println("State: RECORDING");
}

void StateManager::onEnterFileList() {
  Serial.println("State: FILE_LIST");
  // Display will be updated by the DisplayManager
}

void StateManager::processSensorData(float p0, float p1, unsigned long now) {
  // Always update display buffers
  ch0_buffer[buf_index] = p0;
  ch1_buffer[buf_index] = p1;
  
  // Only update display if not in file list mode
  if (displayManager && currentState != FILE_LIST) {
    displayManager->drawOnePoint(buf_index, p0, p1, ch0_buffer, ch1_buffer, buffer_size);
    displayManager->drawPressureText(p0, p1);
  }
  
  // Handle state-specific actions
  handleStateSpecificActions(p0, p1, now);
  
  buf_index = (buf_index + 1) % buffer_size;
}

void StateManager::handleStateSpecificActions(float p0, float p1, unsigned long now) {
  if (currentState == STANDBY) {
    handleStandbyState();
  } else if (currentState == RECORDING) {
    handleRecordingState(p0, p1, now);
  } else if (currentState == FILE_LIST) {
    handleFileListState();
  }
}

void StateManager::handleStandbyState() {
  // In standby: only display data, no logging or MQTT
}

void StateManager::handleRecordingState(float p0, float p1, unsigned long now) {
  // Log data to SD card
  if (sdManager && sdManager->isRecording()) {
    sdManager->logData(p0, p1);
  }
  
  // Publish data via MQTT (at 500ms intervals)
  if (mqttManager && mqttManager->canPublish(now)) {
    mqttManager->publishData(p0, p1);
    mqttManager->updateLastSendTime(now);
  }
}

void StateManager::handleFileListState() {
  // File list state: no sensor data processing needed
  // Navigation and file operations handled by button inputs
}