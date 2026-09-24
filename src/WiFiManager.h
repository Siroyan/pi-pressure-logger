#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <atomic>

class WiFiManager {
private:
  std::atomic<bool> wifi_connected;
  const char* ssid;
  const char* password;
  
public:
  WiFiManager(const char* wifi_ssid, const char* wifi_password);
  
  void init();
  void checkConnection();
  bool isConnected();

};

#endif // WIFI_MANAGER_H
