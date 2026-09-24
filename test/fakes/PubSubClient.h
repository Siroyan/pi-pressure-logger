#pragma once
#include "WiFiClientSecure.h"
#include <functional>
#include <vector>
struct FakeTransport {
  bool bufferAvailable=true,connected=false,connectSuccess=true,publishSuccess=true;
  unsigned connects=0;
  std::vector<std::string> publications,topics;
  std::function<void()> onPublish,onConnect;
};
inline FakeTransport transport;
class PubSubClient {
public:
  explicit PubSubClient(WiFiClientSecure&) {}
  void setServer(const char*,int) {}
  void setSocketTimeout(int) {}
  bool setBufferSize(unsigned) { return transport.bufferAvailable; }
  bool connected() { return transport.connected; }
  bool connect(const char*) {
    ++transport.connects; if (transport.onConnect) transport.onConnect();
    return transport.connected=transport.connectSuccess;
  }
  void disconnect() { transport.connected=false; }
  void loop() {}
  int state() { return -1; }
  bool publish(const char* topic,const char* value) {
    transport.topics.emplace_back(topic); transport.publications.emplace_back(value);
    if (transport.onPublish) transport.onPublish(); return transport.publishSuccess;
  }
};
