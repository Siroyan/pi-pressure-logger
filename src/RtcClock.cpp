#include "RtcClock.h"

bool readRtcTime(struct tm& value) {
  // Arduino getLocalTime(..., 0) still delays 10ms when the clock is unset.
  // Read once with the thread-safe libc API; synchronization runs elsewhere.
  const time_t now=time(nullptr);
  return localtime_r(&now,&value)!=nullptr && value.tm_year>120;
}
