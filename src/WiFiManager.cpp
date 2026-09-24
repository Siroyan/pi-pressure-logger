#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* wifi_ssid, const char* wifi_password)
  : wifi_connected(false), ssid(wifi_ssid), password(wifi_password) {}

void WiFiManager::init() {
  // The SDK retries on disconnect/failure events. An application timer calling
  // reconnect() would force a disconnect even during association or DHCP.
  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  wifi_connected = false;
  checkConnection();
}

void WiFiManager::checkConnection() {
  const bool was_connected = wifi_connected.load();
  // Refresh on every poll, including completion of an asynchronous reconnect.
  wifi_connected = WiFi.status() == WL_CONNECTED;
  if (wifi_connected && !was_connected) {
    Serial.printf("WiFi connected: IP=%s, gateway=%s, DNS1=%s, DNS2=%s\n",
                  WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(),
                  WiFi.dnsIP(0).toString().c_str(), WiFi.dnsIP(1).toString().c_str());
  } else if (!wifi_connected && was_connected) {
    Serial.printf("WiFi not ready, status=%d; waiting for IP or SDK reconnection\n", static_cast<int>(WiFi.status()));
  }
}

bool WiFiManager::isConnected() { return wifi_connected.load(); }
