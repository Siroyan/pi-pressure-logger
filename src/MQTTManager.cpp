#include "MQTTManager.h"
#include <WiFi.h>

namespace {
constexpr uint32_t tcpConnectSeconds = 30;
constexpr uint32_t tlsHandshakeSeconds = 30;
constexpr uint16_t mqttConnectSeconds = 10;
constexpr uint16_t runningIoSeconds = 3;
}

MQTTManager::MQTTManager(WiFiClientSecure* wifi_client, const char* endpoint, int port,
                         const char* name, const char* topic, const char* root_ca,
                         const char* cert, const char* key)
  : wifiClient(wifi_client), client(*wifi_client), endpoint(endpoint), port(port),
    thing(name), topic(topic), root(root_ca), cert(cert), key(key) {}

bool MQTTManager::init() {
  wifiClient->setCACert(root);
  wifiClient->setCertificate(cert);
  wifiClient->setPrivateKey(key);
  wifiClient->setHandshakeTimeout(tlsHandshakeSeconds);
  wifiClient->setTimeout(runningIoSeconds); // This pinned SDK takes seconds.
  client.setServer(endpoint,port);
  client.setSocketTimeout(runningIoSeconds);
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
  portENTER_CRITICAL(&mux);
  const bool lost = status.connected && !connected;
  status.connected=connected;
  portEXIT_CRITICAL(&mux);
  if (lost) Serial.printf("AWS IoT disconnected, MQTT state=%d; reconnecting\n",client.state());
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
    // Connection setup needs more time than established-session I/O. This
    // SDK also uses setTimeout() for TCP connect, so restore it on every retry.
    wifiClient->setTimeout(tcpConnectSeconds);
    client.setSocketTimeout(mqttConnectSeconds);
    const uint32_t connectStarted = millis();
    Serial.printf("Attempting AWS IoT connection... TCP=%lus, TLS=%lus, MQTT=%us\n",
                  static_cast<unsigned long>(tcpConnectSeconds),
                  static_cast<unsigned long>(tlsHandshakeSeconds), mqttConnectSeconds);
    bool connected=client.connect(thing);
    wifiClient->setTimeout(runningIoSeconds);
    client.setSocketTimeout(runningIoSeconds);
    lastConnectAttempt=millis();
    const uint32_t elapsed = lastConnectAttempt-connectStarted;
    updateConnected(connected);
    if (!connected) {
      Serial.printf("AWS connection failed after %lu ms, MQTT state=%d\n",
                    static_cast<unsigned long>(elapsed), client.state());
      Serial.printf("Network: WiFi=%d, IP=%s, gateway=%s, DNS1=%s, DNS2=%s\n",
                    static_cast<int>(WiFi.status()), WiFi.localIP().toString().c_str(),
                    WiFi.gatewayIP().toString().c_str(), WiFi.dnsIP(0).toString().c_str(),
                    WiFi.dnsIP(1).toString().c_str());
      char error[128] = {};
      const int code = wifiClient->lastError(error, sizeof(error));
      // The SDK does not reset this value when hostname resolution fails.
      if (code) Serial.printf("Last TLS error (may be from an earlier attempt): %d (%s)\n", code, error);
      if (client.state() == -2)
        Serial.println("Transport connection failed before MQTT. If 'DNS Failed' appears above, check DNS/network reachability first.");
      return;
    }
    Serial.printf("connected to AWS IoT Core after %lu ms\n", static_cast<unsigned long>(elapsed));
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
  portEXIT_CRITICAL(&mux);
  updateConnected(connected);
  if (!success) Serial.println("MQTT publish failed; latest-value delivery continues");
}
