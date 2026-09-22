#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <freertos/semphr.h>

class MQTTManager {
private:
  WiFiClientSecure* wifiClient;
  PubSubClient* client;
  SemaphoreHandle_t client_mutex;
  bool mqtt_connected;
  unsigned long last_mqtt_attempt;
  unsigned long last_mqtt_send_time;
  const int mqtt_retry_interval = 5000;
  const int mqtt_send_interval = 500;
  const int mqtt_socket_timeout = 3;
  
  const char* aws_iot_endpoint;
  int aws_iot_port;
  const char* thing_name;
  const char* aws_iot_topic;
  const char* aws_root_ca;
  const char* device_cert;
  const char* device_key;
  
public:
  MQTTManager(WiFiClientSecure* wifi_client, const char* endpoint, int port, 
              const char* name, const char* topic, const char* root_ca,
              const char* cert, const char* key);
  
  void init();
  void loop();
  bool isConnected();
  void publishData(float p0, float p1);
  bool canPublish(unsigned long now);
  void updateLastSendTime(unsigned long now);
  
private:
  void reconnect();
  bool isConnectedUnsafe();
};

#endif // MQTT_MANAGER_H
