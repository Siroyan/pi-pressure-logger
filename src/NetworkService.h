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
  // オンラインではMQTT送信バッファの準備状態、オフラインでは常にtrue。
  // Wi-FiやMQTTへの接続状態は示さない。
  bool ready();
  bool enabled() const;
  bool wifiConnected();
  bool mqttConnected();
  // オフライン構成では時計を持たないためnullptrを返す。
  TimeManager* timeSource();
};
