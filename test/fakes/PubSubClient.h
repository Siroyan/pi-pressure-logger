#pragma once
#include "WiFiClientSecure.h"
class PubSubClient {
public:
  explicit PubSubClient(WiFiClientSecure&) {}
  void setServer(const char*,int) {}
  void setSocketTimeout(int) {}
  bool connected() { return false; }
  bool connect(const char*) { return false; }
  void loop() {}
  int state() { return -1; }
  bool publish(const char*,const char*) { return false; }
};
