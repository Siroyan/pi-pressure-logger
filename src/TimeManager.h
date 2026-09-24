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
              long gmt_offset = 9 * 3600,  // 日本標準時（UTC+9）
              int daylight_offset = 0);
  
  // 起動時にWi-Fi接続済みなら同期を試みる。戻り値はその時点で有効な日時があるかどうか。
  bool init();
  // Wi-Fi接続中だけ同期を要求する。要求直後に日時が未確定ならfalseを返す。
  bool syncTime();
  // 通信タスクから定期的に呼ぶ。未同期なら通常30秒間隔で、再接続時はすぐ再要求する。
  void poll();
  // NTPへの接続成否ではなく、RTCから有効な日時を読めるかを判定する。
  bool isTimeSynced();
  
  // 日時が無効な場合は「Time not available」を返す。
  String getCurrentTimeString();
  // YYYY-MM-DD-hh-mm-ss形式。日時が無効な場合は空文字列を返す。
  String getFormattedTimeString();
};

#endif
