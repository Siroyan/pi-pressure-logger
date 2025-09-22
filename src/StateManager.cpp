#include "StateManager.h"
#include "SDManager.h"
#include "MQTTManager.h"
#include "DisplayManager.h"

StateManager::StateManager() : currentState(STANDBY), stateChangeTime(0), 
  sdManager(nullptr), mqttManager(nullptr), displayManager(nullptr) {}

void StateManager::setManagers(SDManager* sd, MQTTManager* mqtt, DisplayManager* display) {
  sdManager = sd;
  mqttManager = mqtt;
  displayManager = display;
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

void StateManager::toggleState() {
  if (currentState == STANDBY) {
    transitionToRecording();
  } else {
    transitionToStandby();
  }
}

void StateManager::onEnterStandby() {
  if (sdManager && sdManager->isRecording()) {
    sdManager->stopRecording();
  }
  Serial.println("State: STANDBY");
}

void StateManager::onEnterRecording() {
  if (sdManager && sdManager->isAvailable()) {
    sdManager->startRecording();
  }
  Serial.println("State: RECORDING");
}

void StateManager::processSensorData(float v0, float v1, unsigned long now) {
  // Always update display buffers and draw
  ch0_buffer[buf_index] = v0;
  ch1_buffer[buf_index] = v1;
  
  if (displayManager) {
    displayManager->drawOnePoint(buf_index, v0, v1, ch0_buffer, ch1_buffer, buffer_size);
    displayManager->drawVoltageText(v0, v1);
  }
  
  // Handle state-specific actions
  handleStateSpecificActions(v0, v1, now);
  
  buf_index = (buf_index + 1) % buffer_size;
}

void StateManager::handleStateSpecificActions(float v0, float v1, unsigned long now) {
  if (currentState == STANDBY) {
    handleStandbyState();
  } else if (currentState == RECORDING) {
    handleRecordingState(v0, v1, now);
  }
}

void StateManager::handleStandbyState() {
  // In standby: only display data, no logging or MQTT
}

void StateManager::handleRecordingState(float v0, float v1, unsigned long now) {
  // Log data to SD card
  if (sdManager && sdManager->isRecording()) {
    sdManager->logData(v0, v1);
  }
  
  // Publish data via MQTT (at 500ms intervals)
  if (mqttManager && mqttManager->canPublish(now)) {
    mqttManager->publishData(v0, v1);
    mqttManager->updateLastSendTime(now);
  }
}