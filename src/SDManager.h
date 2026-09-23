#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <SD.h>
#include <FS.h>
#include <vector>
#include <atomic>
#include "PressureSample.h"

class TimeManager;

class SDManager {
private:
  std::atomic<bool> sd_available;
  String log_filename;
  uint64_t session_start_time;
  std::atomic<bool> recording;
  std::atomic<bool> write_error;
  File log_file;
  char batch[1024];
  size_t batch_size = 0;
  uint32_t last_flush = 0;
  TimeManager* timeManager;
  struct LogEntry { String name; long size; };
  std::vector<LogEntry> file_cache;
  bool cache_valid = false;
  uint64_t boot_number = 0;
  bool refreshFileCache();
  String uptimeFilename();
  
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
  bool flush();
  void poll();
  void writeSummary(uint32_t session, uint32_t dropped, uint32_t high_water);
  
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
  bool append(const String& bytes);
  bool writeBatch();
};

#endif // SD_MANAGER_H
