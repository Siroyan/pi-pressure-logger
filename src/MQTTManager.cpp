#include "MQTTManager.h"

MQTTManager::MQTTManager(WiFiClientSecure* wifi_client, const char* endpoint, int port,
                         const char* name, const char* topic, const char* root_ca,
                         const char* cert, const char* key)
  : wifiClient(wifi_client), client(*wifi_client), endpoint(endpoint), port(port),
    thing(name), topic(topic), root(root_ca), cert(cert), key(key) {}

bool MQTTManager::init() {
  wifiClient->setCACert(root);
  wifiClient->setCertificate(cert);
  wifiClient->setPrivateKey(key);
  wifiClient->setHandshakeTimeout(3);
  wifiClient->setTimeout(3); // This pinned ESP32 SDK takes seconds, not milliseconds.
  client.setServer(endpoint,port);
  client.setSocketTimeout(3);
  bool ready=client.setBufferSize(512);
  portENTER_CRITICAL(&mux); status.ready=ready; portEXIT_CRITICAL(&mux);
  return ready;
}

MQTTStatus MQTTManager::snapshot() {
  portENTER_CRITICAL(&mux); auto copy=status; portEXIT_CRITICAL(&mux); return copy;
}
bool MQTTManager::isReady() { return snapshot().ready; }
bool MQTTManager::isConnected() { return snapshot().connected; }
void MQTTManager::updateConnected(bool connected) {
  portENTER_CRITICAL(&mux); status.connected=connected; portEXIT_CRITICAL(&mux);
}
void MQTTManager::offer(const Telemetry& value) {
  portENTER_CRITICAL(&mux); latest=value; pending=true; portEXIT_CRITICAL(&mux);
}
void MQTTManager::clearPending() {
  portENTER_CRITICAL(&mux); pending=false; portEXIT_CRITICAL(&mux);
}

void MQTTManager::loop(bool network_ready) {
  if (!isReady()) return;
  if (!network_ready) {
    if (client.connected()) client.disconnect();
    updateConnected(false);
    return;
  }
  if (!client.connected()) {
    updateConnected(false);
    if (connectAttempted && static_cast<uint32_t>(millis()-lastConnectAttempt)<5000) return;
    connectAttempted=true;
    bool connected=client.connect(thing);
    lastConnectAttempt=millis();
    updateConnected(connected);
    if (!connected) {
      Serial.printf("AWS connection failed, MQTT state=%d\n",client.state());
      return;
    }
    Serial.println("connected to AWS IoT Core");
  }
  client.loop();
  updateConnected(client.connected());
  if (!client.connected() || (publishAttempted && static_cast<uint32_t>(millis()-lastPublishAttempt)<500)) return;

  Telemetry value;
  portENTER_CRITICAL(&mux);
  bool have=pending;
  if (have) { value=latest; pending=false; }
  portEXIT_CRITICAL(&mux);
  // Latest-value delivery: no history replay, no retry of an old failed sample.
  const uint64_t now=static_cast<uint64_t>(esp_timer_get_time())/1000;
  if (!have || !value.session || value.timestamp>now || now-value.timestamp>1000) return;
  const String payload=telemetryPayload(value,thing);
  bool success=client.publish(topic,payload.c_str());
  bool connected=client.connected();
  publishAttempted=true;
  lastPublishAttempt=millis(); // Throttle from completion, including a stalled/failed send.
  portENTER_CRITICAL(&mux);
  ++status.attempts; status.lastAttempt=lastPublishAttempt;
  if (success) { ++status.successes; status.lastSuccess=lastPublishAttempt; }
  status.connected=connected;
  portEXIT_CRITICAL(&mux);
  if (!success) Serial.println("MQTT publish failed; latest-value delivery continues");
}
