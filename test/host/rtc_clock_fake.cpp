#include "RtcClock.h"
#include "Arduino.h"

bool readRtcTime(struct tm& value) {
  value={};
  if (!fake_time_valid) return false;
  value.tm_year=126; value.tm_mon=8; value.tm_mday=24;
  return true;
}
