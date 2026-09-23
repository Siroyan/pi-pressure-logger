#pragma once
#include "Arduino.h"
class WiFiClientSecure {
public:
  void setCACert(const char*) {}
  void setCertificate(const char*) {}
  void setPrivateKey(const char*) {}
  int lastError(char*,size_t) { return 0; }
};
