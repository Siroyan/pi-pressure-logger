#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* wifi_ssid, const char* wifi_password) 
  : wifi_connected(false), last_wifi_check(0), ssid(wifi_ssid), password(wifi_password) {}

void WiFiManager::init() {
  connect();
}

void WiFiManager::connect() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  unsigned long start_time = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
}

void WiFiManager::checkConnection() {
  unsigned long now = millis();
  if (now - last_wifi_check < wifi_check_interval) {
    return;
  }
  
  last_wifi_check = now;
  
  if (WiFi.status() != WL_CONNECTED && wifi_connected) {
    // WiFi was connected but now disconnected
    wifi_connected = false;
    Serial.println("WiFi disconnected! Attempting reconnection...");
  }
  
  if (!wifi_connected && WiFi.status() != WL_CONNECTED) {
    reconnect();
  }
}

void WiFiManager::reconnect() {
  WiFi.reconnect();
  Serial.print("Reconnecting to WiFi");
  
  unsigned long start_time = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < 5000) {
    delay(100);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println();
    Serial.println("WiFi reconnected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("WiFi reconnection failed, will retry later");
  }
}

bool WiFiManager::isConnected() {
  return wifi_connected && (WiFi.status() == WL_CONNECTED);
}

IPAddress WiFiManager::getLocalIP() {
  return WiFi.localIP();
}