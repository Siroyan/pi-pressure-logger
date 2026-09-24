#pragma once
#include "RecordingQueue.h"
#ifndef PRESSURE_OFFLINE
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "TimeManager.h"
#endif
class TimeManager;

class NetworkService {
#ifndef PRESSURE_OFFLINE
  RecordingQueue& queue;
  WiFiClientSecure tls;
  WiFiManager wifi;
  MQTTManager mqtt;
  TimeManager time;
#endif
public:
  explicit NetworkService(RecordingQueue& samples);
  void init();
  void step();
  bool ready();
  bool enabled() const;
  bool wifiConnected();
  bool mqttConnected();
  TimeManager* timeSource();
};
