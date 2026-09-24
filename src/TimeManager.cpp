#include "TimeManager.h"
#include "RtcClock.h"

TimeManager::TimeManager(const char* server, long gmt_offset, int daylight_offset)
  : ntp_server(server), gmt_offset_sec(gmt_offset), daylight_offset_sec(daylight_offset) {}

bool TimeManager::init() { poll(); return isTimeSynced(); }

bool TimeManager::syncTime() {
  if (WiFi.status() != WL_CONNECTED) return false;
  configTime(gmt_offset_sec, daylight_offset_sec, ntp_server);
  last_attempt = millis();
  requested = true;
  return isTimeSynced();
}

void TimeManager::poll() {
  bool connected = WiFi.status() == WL_CONNECTED;
  bool synced = isTimeSynced();  // Also notices late SNTP success after any timeout.
  if (connected && !synced && (!was_connected || !requested ||
      static_cast<uint32_t>(millis() - last_attempt) >= 30000)) syncTime();
  was_connected = connected;
}

bool TimeManager::isTimeSynced() {
  struct tm info;
  bool valid = readRtcTime(info) && info.tm_year > 120;
  return valid;
}

String TimeManager::getCurrentTimeString() {
  struct tm info;
  if (!readRtcTime(info) || info.tm_year <= 120) return "Time not available";
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &info);
  return String(buffer);
}

String TimeManager::getFormattedTimeString() {
  struct tm info;
  if (!readRtcTime(info) || info.tm_year <= 120) return "";
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d-%H-%M-%S", &info);
  return String(buffer);
}
