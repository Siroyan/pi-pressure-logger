#include "MQTTManager.h"

MQTTManager::MQTTManager(WiFiClientSecure* wifi_client, const char* endpoint, int port, 
                         const char* name, const char* root_ca, const char* cert, const char* key)
  : wifiClient(wifi_client), mqtt_connected(false), last_mqtt_attempt(0), last_mqtt_send_time(0),
    aws_iot_endpoint(endpoint), aws_iot_port(port), thing_name(name),
    aws_root_ca(root_ca), device_cert(cert), device_key(key) {
  
  client = new PubSubClient(*wifiClient);
}

void MQTTManager::init() {
  // Set certificates
  wifiClient->setCACert(aws_root_ca);
  wifiClient->setCertificate(device_cert);
  wifiClient->setPrivateKey(device_key);
  
  client->setServer(aws_iot_endpoint, aws_iot_port);
  
  Serial.println("AWS IoT certificates loaded");
}

void MQTTManager::loop() {
  if (!client->connected()) {
    reconnect();
  }
  client->loop();
}

void MQTTManager::reconnect() {
  if (millis() - last_mqtt_attempt < mqtt_retry_interval) {
    return;
  }
  
  last_mqtt_attempt = millis();
  Serial.print("Attempting AWS IoT connection...");
  
  if (client->connect(thing_name)) {
    mqtt_connected = true;
    Serial.println("connected to AWS IoT Core");
  } else {
    mqtt_connected = false;
    Serial.print("failed, rc=");
    Serial.print(client->state());
    Serial.println(" try again in 5 seconds");
  }
}

bool MQTTManager::isConnected() {
  return mqtt_connected && client->connected();
}

void MQTTManager::publishData(float p0, float p1) {
  if (!isConnected()) return;
  
  String payload = "{";
  payload += "\"timestamp\":" + String(millis());
  payload += ",\"device\":\"" + String(thing_name) + "\"";
  payload += ",\"ch0\":" + String(p0, 4);
  payload += ",\"ch1\":" + String(p1, 4);
  payload += "}";
  
  // Publish to both topics with same data
  bool ch0_published = client->publish("pressure_logger/ch0", payload.c_str());
  bool ch1_published = client->publish("pressure_logger/ch1", payload.c_str());
  
  if (ch0_published && ch1_published) {
    Serial.println("Data published to AWS IoT - CH0&CH1: " + payload);
  } else {
    Serial.println("Failed to publish data");
  }
}

bool MQTTManager::canPublish(unsigned long now) {
  return isConnected() && (now - last_mqtt_send_time >= mqtt_send_interval);
}

void MQTTManager::updateLastSendTime(unsigned long now) {
  last_mqtt_send_time = now;
}