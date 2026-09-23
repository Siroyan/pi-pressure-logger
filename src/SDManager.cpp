#include "SDManager.h"
#include "TimeManager.h"
#include <algorithm>

SDManager::SDManager() : sd_available(false), log_filename(""), session_start_time(0), recording(false), write_error(false), timeManager(nullptr) {}

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

bool SDManager::hasWriteError() {
  return write_error;
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
  
  filename = createUniqueFilename(filename);
  if (filename.length() == 0) {
    Serial.println("Failed to allocate a unique log filename");
    return "";
  }

  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (file) {
    // Write CSV header with timestamp info
    bool complete = writeLine(file, "Timestamp(ms),CH0(MPa),CH1(MPa)");
    
    // Add a comment with session start time if NTP is available
    if (complete && timeManager && timeManager->isTimeSynced()) {
      complete = writeLine(file, "# Session started: " + timeManager->getCurrentTimeString());
    }
    file.flush();
    complete = complete && file.getWriteError() == 0;
    if (!complete) { file.close(); return ""; }
    log_file = file;
    Serial.println("Created log file: " + filename);
    return filename;
  }
  
  Serial.println("Failed to create log file");
  return "";
}

String SDManager::createUniqueFilename(const String& filename) {
  if (!SD.exists(filename.c_str())) {
    return filename;
  }

  const int extensionIndex = filename.lastIndexOf('.');
  const String baseName = extensionIndex >= 0 ? filename.substring(0, extensionIndex) : filename;
  const String extension = extensionIndex >= 0 ? filename.substring(extensionIndex) : "";

  for (unsigned int suffix = 1; suffix < 10000; suffix++) {
    String paddedSuffix = String(suffix);
    while (paddedSuffix.length() < 4) {
      paddedSuffix = "0" + paddedSuffix;
    }

    String candidate = baseName + "_" + paddedSuffix + extension;
    if (!SD.exists(candidate.c_str())) {
      return candidate;
    }
  }

  return "";
}

bool SDManager::startRecording(uint64_t started_at) {
  if (recording) return false;
  recording = false;
  log_filename = "";
  batch_size = 0;

  // A new user-requested session is the only recovery attempt. Keep the
  // failed file for diagnosis; never append to it after remounting.
  if (write_error || !sd_available) {
    SD.end();
    init();
  }
  if (!sd_available) {
    write_error = true;
    return false;
  }
  
  log_filename = createLogFile();
  if (log_filename != "") {
    write_error = false;
    recording = true;
    session_start_time = started_at;
    last_flush = millis();
    Serial.println("Recording started");
    return true;
  }

  failWrite();
  return false;
}

bool SDManager::writeLine(File& file, const String& line) {
  const String bytes = line + "\r\n";
  return file.write(reinterpret_cast<const uint8_t*>(bytes.c_str()), bytes.length()) == bytes.length()
      && file.getWriteError() == 0;
}

void SDManager::failWrite() {
  if (log_file) log_file.close();
  batch_size = 0;
  recording = false;
  write_error = true;
  sd_available = false;
  Serial.println("SD write failed; stop/restart recording to remount and create a new file");
}

void SDManager::stopRecording() {
  if (recording) {
    flush();
    log_file.close();
    recording = false;
    Serial.println("Recording stopped");
  }
}

bool SDManager::logData(const PressureSample& sample) {
  if (!sd_available || log_filename == "" || !recording) return false;
  if (sample.timestamp < session_start_time) return false;
  
  uint64_t timestamp = sample.timestamp - session_start_time;
  String row = String(timestamp) + "," + String(sample.p0, 4) + "," + String(sample.p1, 4);
  return append(row + "\r\n");
}

bool SDManager::append(const String& bytes) {
  if (bytes.length() > sizeof(batch)) { failWrite(); return false; }
  if (batch_size + bytes.length() > sizeof(batch) && !writeBatch()) return false;
  memcpy(batch + batch_size, bytes.c_str(), bytes.length());
  batch_size += bytes.length();
  return true;
}

bool SDManager::writeBatch() {
  if (!recording) return false;
  if (batch_size && (log_file.write(reinterpret_cast<const uint8_t*>(batch), batch_size) != batch_size ||
                     log_file.getWriteError())) {
    failWrite();
    return false;
  }
  batch_size = 0;
  return true;
}

bool SDManager::flush() {
  if (!writeBatch()) return false;
  log_file.flush();
  last_flush = millis();
  if (log_file.getWriteError()) { failWrite(); return false; }
  return true;
}

void SDManager::poll() {
  if (recording && static_cast<uint32_t>(millis() - last_flush) >= 1000) flush();
}

void SDManager::writeSummary(uint32_t session, uint32_t dropped, uint32_t high_water) {
  if (recording) append("# Session: " + String(session) + ", dropped: " + String(dropped) +
                        ", queue_high_water: " + String(high_water) + "\r\n");
}

std::vector<String> SDManager::getLogFileList() {
  std::vector<String> fileList;
  
  if (!sd_available) {
    return fileList;
  }
  
  File root = SD.open("/");
  if (!root) {
    Serial.println("Failed to open root directory");
    return fileList;
  }
  
  File file = root.openNextFile();
  while (file) {
    String filename = file.name();
    // Only include .csv files that start with "pressure_log_"
    if (!file.isDirectory() && filename.endsWith(".csv") && filename.startsWith("pressure_log_")) {
      fileList.push_back(filename);
    }
    file.close();
    file = root.openNextFile();
  }
  root.close();
  
  // Sort files by name (which includes timestamp, so chronological order)
  std::sort(fileList.begin(), fileList.end(), [](const String& a, const String& b) {
    return a > b; // Reverse order - newest first
  });
  
  return fileList;
}

bool SDManager::deleteFile(const String& filename) {
  if (!sd_available) {
    return false;
  }
  
  String fullPath = "/" + filename;
  if (SD.exists(fullPath.c_str())) {
    bool result = SD.remove(fullPath.c_str());
    if (result) {
      Serial.println("Deleted file: " + filename);
    } else {
      Serial.println("Failed to delete file: " + filename);
    }
    return result;
  } else {
    Serial.println("File not found: " + filename);
    return false;
  }
}

long SDManager::getFileSize(const String& filename) {
  if (!sd_available) {
    return -1;
  }
  
  String fullPath = "/" + filename;
  File file = SD.open(fullPath.c_str(), FILE_READ);
  if (file) {
    long size = file.size();
    file.close();
    return size;
  }
  
  return -1;
}

String SDManager::getFileTimestamp(const String& filename) {
  // Extract timestamp from filename
  // Format: pressure_log_YYYY-MM-DD-hh-mm-ss.csv or pressure_log_millis.csv
  
  String name = filename;
  name.replace("pressure_log_", "");
  name.replace(".csv", "");
  
  // Check if it's a timestamp format (contains hyphens)
  if (name.indexOf('-') >= 0) {
    // Convert YYYY-MM-DD-hh-mm-ss to readable format
    name.replace('-', '/');
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) {
      name = name.substring(0, lastSlash) + " " + name.substring(lastSlash + 1);
      name.replace('/', ':');
    }
    return name;
  } else {
    // Millis-based filename
    return "Legacy (" + name + ")";
  }
}
