#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <SD.h>
#include <FS.h>
#include <vector>
#include "PressureSample.h"

class TimeManager;

class SDManager {
private:
  bool sd_available;
  String log_filename;
  uint64_t session_start_time;
  bool recording;
  bool write_error;
  TimeManager* timeManager;
  
public:
  SDManager();
  
  void setTimeManager(TimeManager* tm);
  bool init();
  bool isAvailable();
  bool isRecording();
  bool hasWriteError();
  
  bool startRecording(uint64_t started_at);
  void stopRecording();
  bool logData(const PressureSample& sample);
  
  // File management functions
  std::vector<String> getLogFileList();
  bool deleteFile(const String& filename);
  long getFileSize(const String& filename);
  String getFileTimestamp(const String& filename);
  
private:
  String createLogFile();
  String createUniqueFilename(const String& filename);
  bool writeLine(File& file, const String& line);
  void failWrite();
};

#endif // SD_MANAGER_H
