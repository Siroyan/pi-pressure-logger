#include "Test.h"
#include "WiFiManager.h"

TEST(wifi_async_reconnect_refreshes_status_without_waiting) {
  WiFi={}; fake_millis=100; Serial.output.clear();
  WiFiManager wifi("test","test"); wifi.init();
  CHECK(fake_millis==100 && !wifi.isConnected());
  CHECK(WiFi.autoReconnect);
  fake_millis=5100; wifi.checkConnection(); CHECK(WiFi.reconnects==0);
  CHECK(fake_millis==5100 && !wifi.isConnected());
  WiFi.state=WL_CONNECTED; ++fake_millis; wifi.checkConnection();
  CHECK(wifi.isConnected()); CHECK(WiFi.reconnects==0);
  CHECK(Serial.output.find("WiFi connected: IP=")!=std::string::npos);
  Serial.output.clear(); wifi.checkConnection(); CHECK(Serial.output.empty());
  WiFi.connectionFailed(); wifi.checkConnection(); CHECK(!wifi.isConnected());
  CHECK(WiFi.automaticReconnects==1);
  CHECK(Serial.output.find("WiFi not ready, status=6")!=std::string::npos);
  fake_millis=10100; wifi.checkConnection(); CHECK(WiFi.reconnects==0);
  WiFi.state=WL_CONNECTED; wifi.checkConnection(); CHECK(wifi.isConnected());
}

TEST(wifi_clock_wrap_does_not_restart_an_in_progress_connection) {
  WiFi={}; fake_millis=UINT32_MAX-100;
  WiFiManager wifi("test","test"); wifi.init();
  fake_millis=50; wifi.checkConnection(); CHECK(WiFi.reconnects==0);
  fake_millis=5000; wifi.checkConnection(); CHECK(WiFi.reconnects==0);
  CHECK(WiFi.begins==1);
  WiFi.state=WL_CONNECTED; wifi.checkConnection(); CHECK(wifi.isConnected());
}

TEST(wifi_slow_association_and_dhcp_are_not_interrupted_by_polling) {
  WiFi={}; fake_millis=0;
  WiFiManager wifi("test","test"); wifi.init();
  // Both association and DHCP can exceed the former five-second retry period.
  for (unsigned now=5000; now<=120000; now+=5000) {
    if (now==30000) WiFi.state=WL_IDLE_STATUS; // Associated, but awaiting DHCP.
    fake_millis=now; wifi.checkConnection();
    CHECK(fake_millis==now && !wifi.isConnected());
    CHECK(WiFi.begins==1 && WiFi.reconnects==0 && WiFi.automaticReconnects==0);
  }
  WiFi.state=WL_CONNECTED; wifi.checkConnection(); CHECK(wifi.isConnected());
  // Loss of IP without a disconnect must also allow DHCP to finish.
  WiFi.state=WL_IDLE_STATUS;
  for (unsigned now=125000; now<=150000; now+=5000) {
    fake_millis=now; wifi.checkConnection(); CHECK(!wifi.isConnected());
    CHECK(WiFi.begins==1 && WiFi.reconnects==0);
  }
  WiFi.state=WL_CONNECTED; wifi.checkConnection(); CHECK(wifi.isConnected());
}

TEST(wifi_failed_attempts_use_sdk_retry_without_a_second_application_retry) {
  WiFi={}; fake_millis=0;
  WiFiManager wifi("test","test"); wifi.init();
  for (unsigned attempt=1; attempt<=3; ++attempt) {
    fake_millis+=10000; WiFi.connectionFailed(); wifi.checkConnection();
    CHECK(WiFi.automaticReconnects==attempt);
    fake_millis+=6000; wifi.checkConnection();
    CHECK(WiFi.begins==1 && WiFi.reconnects==0);
  }
  WiFi.state=WL_CONNECTED; wifi.checkConnection(); CHECK(wifi.isConnected());
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
