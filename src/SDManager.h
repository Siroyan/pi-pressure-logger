#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <SD.h>
#include <FS.h>

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
  void logData(float v0, float v1);
  
private:
  String createLogFile();
};

#endif // SD_MANAGER_H