#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <SD.h>
#include <FS.h>
#include <vector>

class TimeManager;

class SDManager {
private:
  bool sd_available;
  String log_filename;
  unsigned long session_start_time;
  bool recording;
  TimeManager* timeManager;
  
public:
  SDManager();
  
  void setTimeManager(TimeManager* tm);
  bool init();
  bool isAvailable();
  bool isRecording();
  
  void startRecording();
  void stopRecording();
  void logData(float p0, float p1);
  
  // File management functions
  std::vector<String> getLogFileList();
  bool deleteFile(const String& filename);
  long getFileSize(const String& filename);
  String getFileTimestamp(const String& filename);
  
private:
  String createLogFile();
};

#endif // SD_MANAGER_H