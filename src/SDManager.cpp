#include "SDManager.h"
#include "TimeManager.h"

SDManager::SDManager() : sd_available(false), log_filename(""), session_start_time(0), recording(false), timeManager(nullptr) {}

void SDManager::setTimeManager(TimeManager* tm) {
  timeManager = tm;
}

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
  String filename;
  
  // Use NTP time for filename if available
  if (timeManager && timeManager->isTimeSynced()) {
    String timeString = timeManager->getFormattedTimeString();
    if (timeString.length() > 0) {
      filename = "/pressure_log_" + timeString + ".csv";
    } else {
      // If getFormattedTimeString returns empty, fall back to millis
      Serial.println("Failed to get formatted time, using millis fallback");
      filename = "/pressure_log_" + String(millis()) + ".csv";
    }
  } else {
    // Fallback to millis-based naming
    Serial.println("NTP time not available, using millis-based filename");
    unsigned long timestamp = millis();
    filename = "/pressure_log_" + String(timestamp) + ".csv";
  }
  
  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (file) {
    // Write CSV header with timestamp info
    file.println("Timestamp(ms),CH0(V),CH1(V)");
    
    // Add a comment with session start time if NTP is available
    if (timeManager && timeManager->isTimeSynced()) {
      file.println("# Session started: " + timeManager->getCurrentTimeString());
    }
    
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