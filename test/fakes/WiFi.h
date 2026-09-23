#pragma once
#include "Arduino.h"
constexpr int WL_CONNECTED = 3;
struct IPAddress {};
struct FakeWiFi { int state=0; int status() { return state; } };
inline FakeWiFi WiFi;
