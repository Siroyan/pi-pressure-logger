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

#include "TimeManager.h"
TEST(ntp_offline_start_and_recovery_never_wait_for_time) {
  WiFi={}; fake_time_valid=false; fake_millis=0; fake_ntp_requests=0;
  TimeManager time; CHECK(!time.init()); CHECK(fake_millis==0 && fake_ntp_requests==0);
  WiFi.state=WL_CONNECTED; time.poll(); CHECK(fake_ntp_requests==1 && fake_millis==0);
  for (unsigned i=1;i<300;++i) { fake_millis=i*100; time.poll(); CHECK(fake_millis==i*100); }
  CHECK(fake_ntp_requests==1);
  fake_millis=30000; time.poll(); CHECK(fake_ntp_requests==2 && fake_millis==30000);
  fake_time_valid=true; time.poll(); CHECK(time.isTimeSynced());
  CHECK(time.getFormattedTimeString()=="2026-09-24-00-00-00");
  fake_time_valid=false;
}

TEST(ntp_late_success_and_wifi_recovery_are_observed_immediately) {
  WiFi={}; WiFi.state=WL_CONNECTED; fake_time_valid=false; fake_millis=100; fake_ntp_requests=0;
  TimeManager time; time.init();
  fake_millis=15000; fake_time_valid=true; CHECK(time.isTimeSynced());
  CHECK(fake_ntp_requests==1);
  fake_time_valid=false; WiFi.state=0; time.poll();
  WiFi.state=WL_CONNECTED; ++fake_millis; time.poll(); CHECK(fake_ntp_requests==2);
  CHECK(time.getFormattedTimeString()==""); CHECK(fake_millis==15001);
}
