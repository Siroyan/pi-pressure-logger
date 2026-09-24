#pragma once
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include "Telemetry.h"

struct MQTTStatus {
  bool ready=false, connected=false;
  uint32_t attempts=0, successes=0, lastAttempt=0, lastSuccess=0;
};

class MQTTManager {
  WiFiClientSecure* wifiClient;
  PubSubClient client; // 通信タスクだけがこのクライアントの入出力を行う。
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  Telemetry latest;
  bool pending=false;
  MQTTStatus status;
  bool connectAttempted=false;
  uint32_t lastConnectAttempt=0;
  bool publishAttempted=false;
  uint32_t lastPublishAttempt=0;
  const char* endpoint;
  int port;
  const char* thing;
  const char* topic;
  const char* root;
  const char* cert;
  const char* key;
  void updateConnected(bool connected);
public:
  MQTTManager(WiFiClientSecure* wifi_client, const char* endpoint, int port,
              const char* name, const char* topic, const char* root_ca,
              const char* cert, const char* key);
  // 送信バッファを確保する。trueはMQTT接続済みを意味しない。
  bool init();
  bool isReady();
  bool isConnected();
  MQTTStatus snapshot();
  // 送信待ちの値を最新の1件で上書きする。古い値を蓄積しない。
  void offer(const Telemetry& value);
  void clearPending();
  // 通信タスクから呼ぶ。接続処理では同期的に待つため、画面側からは呼ばない。
  // network_readyがfalseなら既存の接続を切り、1秒を超えた値は送信しない。
  void loop(bool network_ready=true);
};
