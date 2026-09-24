#pragma once
#include "Arduino.h"
constexpr int WL_CONNECTED = 3;
constexpr int WL_IDLE_STATUS = 0, WL_DISCONNECTED = 6;
struct IPAddress {
  String value="0.0.0.0";
  IPAddress() = default;
  IPAddress(const char* address) : value(address) {}
  String toString() const { return value; }
};
struct FakeWiFi {
  int state=WL_DISCONNECTED; unsigned begins=0, reconnects=0;
  bool autoReconnect=false;
  unsigned automaticReconnects=0;
  IPAddress local, gateway, dns[2];
  int status() { return state; }
  void begin(const char*,const char*) { ++begins; }
  void reconnect() { ++reconnects; }
  void setAutoReconnect(bool enabled) { autoReconnect=enabled; }
  // Model the SDK's disconnect event handler, not an application poll retry.
  void connectionFailed() {
    state=WL_DISCONNECTED;
    if (autoReconnect) ++automaticReconnects;
  }
  IPAddress localIP() { return local; }
  IPAddress gatewayIP() { return gateway; }
  IPAddress dnsIP(unsigned index) { return dns[index]; }
};
inline FakeWiFi WiFi;
