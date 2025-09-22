#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <WiFi.h>
#include <time.h>

class TimeManager {
private:
  bool time_synced;
  const char* ntp_server;
  long gmt_offset_sec;
  int daylight_offset_sec;
  
public:
  TimeManager(const char* ntp_server = "pool.ntp.org", 
              long gmt_offset = 9 * 3600,  // JST (GMT+9)
              int daylight_offset = 0);
  
  bool init();
  bool syncTime();
  bool isTimeSynced();
  
  String getCurrentTimeString();
  String getFormattedTimeString();  // Returns YYYY-MM-DD-hh-mm-ss format
  
  void printCurrentTime();
  
private:
  bool waitForTimeSync(int timeout_seconds = 10);
};

#endif // TIME_MANAGER_H