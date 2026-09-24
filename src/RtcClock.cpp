#include "RtcClock.h"

bool readRtcTime(struct tm& value) {
  // 時計が未設定でもArduinoのgetLocalTime(..., 0)は10ミリ秒待つ。
  // 同期処理は別で行い、ここではスレッド安全な関数で時刻を一度だけ読む。
  const time_t now=time(nullptr);
  return localtime_r(&now,&value)!=nullptr && value.tm_year>120;
}
