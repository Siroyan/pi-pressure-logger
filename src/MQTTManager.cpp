#include "MQTTManager.h"

MQTTManager::MQTTManager(WiFiClientSecure* wifi_client, const char* endpoint, int port, 
                         const char* name, const char* topic, const char* root_ca,
                         const char* cert, const char* key)
  : wifiClient(wifi_client), client_mutex(xSemaphoreCreateRecursiveMutex()), mqtt_connected(false), last_mqtt_attempt(0), last_mqtt_send_time(0),
    aws_iot_endpoint(endpoint), aws_iot_port(port), thing_name(name), aws_iot_topic(topic),
    aws_root_ca(root_ca), device_cert(cert), device_key(key) {
  
  client = new PubSubClient(*wifiClient);
}

void MQTTManager::init() {
  // Set certificates
  wifiClient->setCACert(aws_root_ca);
  wifiClient->setCertificate(device_cert);
  wifiClient->setPrivateKey(device_key);
  
  client->setServer(aws_iot_endpoint, aws_iot_port);
  client->setSocketTimeout(mqtt_socket_timeout);
  
  Serial.println("AWS IoT certificates loaded");
}

void MQTTManager::loop() {
  xSemaphoreTakeRecursive(client_mutex, portMAX_DELAY);
  if (!client->connected()) {
    reconnect();
  }
  client->loop();
  xSemaphoreGiveRecursive(client_mutex);
}

void MQTTManager::reconnect() {
  if (millis() - last_mqtt_attempt < mqtt_retry_interval) {
    return;
  }
  
  Serial.print("Attempting AWS IoT connection...");
  
  if (client->connect(thing_name)) {
    mqtt_connected = true;
    Serial.println("connected to AWS IoT Core");
  } else {
    mqtt_connected = false;
    Serial.print("failed, rc=");
    Serial.print(client->state());
    Serial.println(" try again in 5 seconds");

    char tls_error[128];
    int tls_error_code = wifiClient->lastError(tls_error, sizeof(tls_error));
    if (tls_error_code != 0) {
      Serial.print("TLS error: ");
      Serial.print(tls_error_code);
      Serial.print(" (");
      Serial.print(tls_error);
      Serial.println(")");
    }
  }
  last_mqtt_attempt = millis();
}

bool MQTTManager::isConnected() {
  if (xSemaphoreTakeRecursive(client_mutex, 0) != pdTRUE) {
    return mqtt_connected;
  }
  bool connected = isConnectedUnsafe();
  xSemaphoreGiveRecursive(client_mutex);
  return connected;
}

bool MQTTManager::isConnectedUnsafe() {
  return mqtt_connected && client->connected();
}

void MQTTManager::publishData(float p0, float p1) {
  if (xSemaphoreTakeRecursive(client_mutex, 0) != pdTRUE) {
    return;
  }
  if (!isConnectedUnsafe()) {
    xSemaphoreGiveRecursive(client_mutex);
    return;
  }
  
  String payload = "{";
  payload += "\"timestamp\":" + String(millis());
  payload += ",\"device\":\"" + String(thing_name) + "\"";
  payload += ",\"ch0\":" + String(p0, 4);
  payload += ",\"ch1\":" + String(p1, 4);
  payload += "}";
  
  bool published = client->publish(aws_iot_topic, payload.c_str());
  
  if (published) {
    Serial.println("Data published to AWS IoT [" + String(aws_iot_topic) + "]: " + payload);
  } else {
    Serial.println("Failed to publish data to AWS IoT [" + String(aws_iot_topic) + "]");
  }
  xSemaphoreGiveRecursive(client_mutex);
}

bool MQTTManager::canPublish(unsigned long now) {
  if (xSemaphoreTakeRecursive(client_mutex, 0) != pdTRUE) {
    return false;
  }
  bool can_publish = isConnectedUnsafe() && (now - last_mqtt_send_time >= mqtt_send_interval);
  xSemaphoreGiveRecursive(client_mutex);
  return can_publish;
}

void MQTTManager::updateLastSendTime(unsigned long now) {
  if (xSemaphoreTakeRecursive(client_mutex, 0) != pdTRUE) {
    return;
  }
  last_mqtt_send_time = now;
  xSemaphoreGiveRecursive(client_mutex);
}
