#pragma once
#include "Arduino.h"
class WiFiClientSecure {
public:
  int errorCode=0;
  unsigned handshakeSeconds=120, socketSeconds=30;
  void setCACert(const char*) {}
  void setCertificate(const char*) {}
  void setPrivateKey(const char*) {}
  void setHandshakeTimeout(unsigned seconds) { handshakeSeconds=seconds; }
  void setTimeout(unsigned seconds) { socketSeconds=seconds; }
  int lastError(char* message,size_t length) {
    if (errorCode) std::snprintf(message,length,"fake TLS error");
    return errorCode;
  }
};
