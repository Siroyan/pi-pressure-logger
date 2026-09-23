#include "Test.h"
#include "WiFiManager.h"

TEST(wifi_async_reconnect_refreshes_status_without_waiting) {
  WiFi={}; fake_millis=100;
  WiFiManager wifi("test","test"); wifi.init();
  CHECK(fake_millis==100 && !wifi.isConnected());
  fake_millis=5100; wifi.checkConnection(); CHECK(WiFi.reconnects==1);
  CHECK(fake_millis==5100 && !wifi.isConnected());
  WiFi.state=WL_CONNECTED; ++fake_millis; wifi.checkConnection();
  CHECK(wifi.isConnected()); CHECK(WiFi.reconnects==1);
  WiFi.state=0; wifi.checkConnection(); CHECK(!wifi.isConnected());
  fake_millis=10100; wifi.checkConnection(); CHECK(WiFi.reconnects==2);
}

TEST(wifi_retry_interval_handles_clock_wrap) {
  WiFi={}; fake_millis=UINT32_MAX-100;
  WiFiManager wifi("test","test"); wifi.init();
  fake_millis=50; wifi.checkConnection(); CHECK(WiFi.reconnects==0);
  fake_millis=5000; wifi.checkConnection(); CHECK(WiFi.reconnects==1);
}
