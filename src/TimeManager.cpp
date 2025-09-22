#include "TimeManager.h"

TimeManager::TimeManager(const char* server, long gmt_offset, int daylight_offset)
  : time_synced(false), ntp_server(server), gmt_offset_sec(gmt_offset), daylight_offset_sec(daylight_offset) {}

bool TimeManager::init() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - cannot sync time");
    return false;
  }
  
  return syncTime();
}

bool TimeManager::syncTime() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected - cannot sync time");
    return false;
  }
  
  Serial.println("Synchronizing time with NTP server...");
  
  // Configure time with NTP server
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  
  // Wait for time synchronization
  time_synced = waitForTimeSync();
  
  if (time_synced) {
    Serial.println("Time synchronized successfully");
    printCurrentTime();
  } else {
    Serial.println("Time synchronization failed");
  }
  
  return time_synced;
}

bool TimeManager::waitForTimeSync(int timeout_seconds) {
  struct tm timeinfo;
  int attempts = 0;
  
  while (attempts < timeout_seconds) {
    if (getLocalTime(&timeinfo)) {
      // Check if we got a reasonable year (after 2020)
      if (timeinfo.tm_year > 120) {  // tm_year is years since 1900
        return true;
      }
    }
    delay(1000);
    attempts++;
    Serial.print(".");
  }
  
  Serial.println();
  return false;
}

bool TimeManager::isTimeSynced() {
  if (!time_synced) {
    return false;
  }
  
  struct tm timeinfo;
  return getLocalTime(&timeinfo) && (timeinfo.tm_year > 120);
}

String TimeManager::getCurrentTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Time not available";
  }
  
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

String TimeManager::getFormattedTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "";  // Return empty string if time not available
  }
  
  char buffer[32];
  // Format: YYYY-MM-DD-hh-mm-ss
  strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", &timeinfo);
  
  return String(buffer);
}

void TimeManager::printCurrentTime() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    Serial.println("Current time: " + getCurrentTimeString());
  } else {
    Serial.println("Failed to obtain time");
  }
}