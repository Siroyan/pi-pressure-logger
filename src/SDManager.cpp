#include "SDManager.h"

SDManager::SDManager() : sd_available(false), log_filename(""), session_start_time(0), recording(false) {}

bool SDManager::init() {
  if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
    sd_available = false;
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    sd_available = false;
    return false;
  }
  
  sd_available = true;
  Serial.println("SD Card initialized successfully");
  return true;
}

bool SDManager::isAvailable() {
  return sd_available;
}

bool SDManager::isRecording() {
  return recording;
}

String SDManager::createLogFile() {
  // Create filename with timestamp
  unsigned long timestamp = millis();
  String filename = "/pressure_log_" + String(timestamp) + ".csv";
  
  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (file) {
    // Write CSV header
    file.println("Timestamp(ms),CH0(V),CH1(V)");
    file.close();
    Serial.println("Created log file: " + filename);
    return filename;
  }
  
  Serial.println("Failed to create log file");
  return "";
}

void SDManager::startRecording() {
  if (!sd_available) return;
  
  log_filename = createLogFile();
  if (log_filename != "") {
    recording = true;
    session_start_time = millis();
    Serial.println("Recording started");
  }
}

void SDManager::stopRecording() {
  if (recording) {
    recording = false;
    Serial.println("Recording stopped");
  }
}

void SDManager::logData(float v0, float v1) {
  if (!sd_available || log_filename == "" || !recording) return;
  
  File file = SD.open(log_filename.c_str(), FILE_APPEND);
  if (file) {
    unsigned long timestamp = millis() - session_start_time;
    file.println(String(timestamp) + "," + String(v0, 3) + "," + String(v1, 3));
    file.close();
  }
}