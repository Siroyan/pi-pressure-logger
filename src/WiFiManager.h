#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <atomic>

class WiFiManager {
private:
  std::atomic<bool> wifi_connected;
  uint32_t last_wifi_check;
  const int wifi_check_interval = 5000; // Check WiFi connection every 5 seconds
  const char* ssid;
  const char* password;
  
public:
  WiFiManager(const char* wifi_ssid, const char* wifi_password);
  
  void init();
  void checkConnection();
  bool isConnected();
  IPAddress getLocalIP();

};

#endif // WIFI_MANAGER_H
