#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <WiFi.h>
#include <time.h>

class TimeManager {
private:
  bool requested = false, was_connected = false;
  uint32_t last_attempt = 0;
  const char* ntp_server;
  long gmt_offset_sec;
  int daylight_offset_sec;
  
public:
  TimeManager(const char* ntp_server = "pool.ntp.org", 
              long gmt_offset = 9 * 3600,  // JST (GMT+9)
              int daylight_offset = 0);
  
  bool init();
  bool syncTime();
  void poll();
  bool isTimeSynced();
  
  String getCurrentTimeString();
  String getFormattedTimeString();  // Returns YYYY-MM-DD-hh-mm-ss format
  
  void printCurrentTime();

};

#endif // TIME_MANAGER_H