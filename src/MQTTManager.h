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
  PubSubClient client; // The network task is the sole owner of client I/O.
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
  bool init();
  bool isReady();
  bool isConnected();
  MQTTStatus snapshot();
  void offer(const Telemetry& value);
  void clearPending();
  void loop(bool network_ready=true);
};
