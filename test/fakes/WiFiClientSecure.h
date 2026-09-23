#pragma once
#include "Arduino.h"
class WiFiClientSecure {
public:
  void setCACert(const char*) {}
  void setCertificate(const char*) {}
  void setPrivateKey(const char*) {}
  void setHandshakeTimeout(unsigned) {}
  void setTimeout(unsigned) {}
  int lastError(char*,size_t) { return 0; }
};
