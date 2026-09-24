#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* wifi_ssid, const char* wifi_password)
  : wifi_connected(false), last_wifi_check(0), ssid(wifi_ssid), password(wifi_password) {}

void WiFiManager::init() {
  WiFi.begin(ssid, password);
  last_wifi_check = millis();
  wifi_connected = WiFi.status() == WL_CONNECTED;
}

void WiFiManager::checkConnection() {
  // Refresh on every poll, including completion of an asynchronous reconnect.
  wifi_connected = WiFi.status() == WL_CONNECTED;
  uint32_t now = millis();
  if (!wifi_connected && static_cast<uint32_t>(now - last_wifi_check) >= wifi_check_interval) {
    last_wifi_check = now;
    WiFi.reconnect();
    wifi_connected = WiFi.status() == WL_CONNECTED;
  }
}

bool WiFiManager::isConnected() { return wifi_connected.load(); }
IPAddress WiFiManager::getLocalIP() { return WiFi.localIP(); }
