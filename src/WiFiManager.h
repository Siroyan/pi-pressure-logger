#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

class WiFiManager {
private:
  bool wifi_connected;
  unsigned long last_wifi_check;
  const int wifi_check_interval = 5000; // Check WiFi connection every 5 seconds
  const char* ssid;
  const char* password;
  
public:
  WiFiManager(const char* wifi_ssid, const char* wifi_password);
  
  void init();
  void checkConnection();
  bool isConnected();
  IPAddress getLocalIP();
  
private:
  void connect();
  void reconnect();
};

#endif // WIFI_MANAGER_H