#pragma once
#include "WiFiClientSecure.h"
#include <esp_timer.h>
#include <functional>
#include <vector>
struct FakeTransport {
  bool bufferAvailable=true,connected=false,connectSuccess=true,publishSuccess=true;
  unsigned connects=0;
  int connectionState=-1;
  unsigned tcpDelayMs=0, tlsDelayMs=0, connackDelayMs=0;
  std::vector<std::string> publications,topics;
  std::function<void()> onPublish,onConnect;
};
inline FakeTransport transport;
class PubSubClient {
  WiFiClientSecure& tls;
  unsigned socketSeconds=15;
  bool waitFor(unsigned delayMs, unsigned timeoutSeconds, int failureState) {
    const unsigned elapsed=std::min(delayMs,timeoutSeconds*1000);
    fake_millis+=elapsed; fake_acquisition_ms+=elapsed;
    if (delayMs>timeoutSeconds*1000) { transport.connectionState=failureState; return false; }
    return true;
  }
public:
  explicit PubSubClient(WiFiClientSecure& client) : tls(client) {}
  void setServer(const char*,int) {}
  void setSocketTimeout(int seconds) { socketSeconds=seconds; }
  bool setBufferSize(unsigned) { return transport.bufferAvailable; }
  bool connected() { return transport.connected; }
  bool connect(const char*) {
    ++transport.connects; if (transport.onConnect) transport.onConnect();
    return transport.connected=transport.connectSuccess &&
      waitFor(transport.tcpDelayMs,tls.socketSeconds,-2) &&
      waitFor(transport.tlsDelayMs,tls.handshakeSeconds,-2) &&
      waitFor(transport.connackDelayMs,socketSeconds,-4);
  }
  void disconnect() { transport.connected=false; }
  void loop() {}
  int state() { return transport.connectionState; }
  bool publish(const char* topic,const char* value) {
    transport.topics.emplace_back(topic); transport.publications.emplace_back(value);
    if (transport.onPublish) transport.onPublish(); return transport.publishSuccess;
  }
};
