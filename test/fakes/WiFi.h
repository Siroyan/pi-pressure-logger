#pragma once
#include "Arduino.h"
constexpr int WL_CONNECTED = 3;
struct IPAddress {};
struct FakeWiFi {
  int state=0; unsigned begins=0, reconnects=0;
  int status() { return state; }
  void begin(const char*,const char*) { ++begins; }
  void reconnect() { ++reconnects; }
  IPAddress localIP() { return {}; }
};
inline FakeWiFi WiFi;
