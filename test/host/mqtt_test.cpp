#include "Test.h"
#include "MQTTManager.h"
#include <WiFi.h>

TEST(mqtt_slow_connection_succeeds_discards_stale_sample_and_restores_io_timeout) {
  transport={}; fake_millis=1000; fake_acquisition_ms=1000;
  WiFiClientSecure tls;
  MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","",""); CHECK(mqtt.init());
  transport.tcpDelayMs=5000; transport.tlsDelayMs=6000; transport.connackDelayMs=4000;
  mqtt.offer({0.1f,0.2f,1000,1,1}); mqtt.loop();
  CHECK(mqtt.isConnected()); CHECK(fake_millis==16000);
  CHECK(tls.socketSeconds==3 && transport.publications.empty());
  mqtt.offer({0.1f,0.2f,fake_acquisition_ms,1,2}); mqtt.loop();
  CHECK(transport.publications.size()==1);
  // Reconnection must restore the longer connection timeout each time.
  fake_millis=21000; fake_acquisition_ms=21000;
  transport.connected=false; mqtt.loop();
  CHECK(mqtt.isConnected() && transport.connects==2 && fake_millis==36000);
  CHECK(tls.socketSeconds==3);
}

TEST(mqtt_connection_deadlines_remain_finite_and_retry_from_completion) {
  for (unsigned phase=0; phase<3; ++phase) {
    transport={}; fake_millis=1000; fake_acquisition_ms=1000;
    WiFiClientSecure tls;
    MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","",""); CHECK(mqtt.init());
    if (phase==0) transport.tcpDelayMs=60000;
    if (phase==1) transport.tlsDelayMs=60000;
    if (phase==2) transport.connackDelayMs=60000;
    mqtt.loop(); CHECK(!mqtt.isConnected());
    const unsigned finished=fake_millis;
    CHECK(finished>1000 && finished<=31000 && tls.socketSeconds==3);
    fake_millis=finished+4999; mqtt.loop(); CHECK(transport.connects==1);
    transport.tcpDelayMs=transport.tlsDelayMs=transport.connackDelayMs=0;
    fake_millis=finished+5000; mqtt.loop(); CHECK(mqtt.isConnected() && transport.connects==2);
  }
}

TEST(mqtt_transport_failure_reports_network_and_retries_without_losing_latest_value) {
  transport={}; WiFi={}; Serial.output.clear();
  fake_millis=1000; fake_acquisition_ms=1000;
  WiFi.state=WL_CONNECTED; WiFi.local="192.0.2.2"; WiFi.gateway="192.0.2.1";
  WiFi.dns[0]="192.0.2.53";
  WiFiClientSecure tls;
  MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","",""); CHECK(mqtt.init());
  transport.connectSuccess=false; transport.connectionState=-2;
  transport.onConnect=[] { fake_millis+=7000; fake_acquisition_ms+=7000; };
  mqtt.loop(); CHECK(!mqtt.isConnected());
  CHECK(Serial.output.find("DNS1=192.0.2.53, DNS2=0.0.0.0")!=std::string::npos);
  CHECK(Serial.output.find("IP=192.0.2.2, gateway=192.0.2.1")!=std::string::npos);
  CHECK(Serial.output.find("before MQTT")!=std::string::npos);
  CHECK(Serial.output.find("Last TLS error")==std::string::npos);
  Serial.output.clear();
  fake_millis=12999; mqtt.loop(); CHECK(transport.connects==1 && Serial.output.empty());
  tls.errorCode=-1234; fake_millis=13000; mqtt.loop();
  CHECK(transport.connects==2);
  CHECK(Serial.output.find("may be from an earlier attempt")!=std::string::npos);
  transport.onConnect={}; transport.connectSuccess=true;
  fake_millis=25000; fake_acquisition_ms=25000;
  mqtt.offer({0.1f,0.2f,25000,1,42}); mqtt.loop();
  CHECK(mqtt.isConnected() && transport.publications.size()==1);
}

TEST(mqtt_failure_throttles_attempts_without_claiming_success) {
  transport={}; fake_millis=1000; fake_acquisition_ms=1000;
  WiFiClientSecure tls; MQTTManager mqtt(&tls,"example.invalid",8883,"test","pressure_logger/data","","","");
  CHECK(mqtt.init()); transport.publishSuccess=false;
  mqtt.offer({0.6f,-0.1f,990,1,1}); mqtt.loop();
  CHECK(mqtt.snapshot().attempts==1 && mqtt.snapshot().successes==0 && mqtt.snapshot().lastSuccess==0);
  mqtt.offer({0.2f,0.3f,1000,1,2}); fake_millis=1499; fake_acquisition_ms=1499; mqtt.loop();
  CHECK(transport.publications.size()==1);
  fake_millis=1500; fake_acquisition_ms=1500; transport.publishSuccess=true; mqtt.loop();
  auto status=mqtt.snapshot(); CHECK(status.attempts==2 && status.successes==1 && status.lastSuccess==1500);
  CHECK(transport.topics.back()=="pressure_logger/data");
  CHECK(transport.publications.back().find("\"timestamp\":1000")!=std::string::npos);
}

TEST(mqtt_stall_allows_new_offers_and_uses_wall_time_not_sample_backlog) {
  transport={}; fake_millis=1000; fake_acquisition_ms=1000;
  WiFiClientSecure tls; MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","",""); CHECK(mqtt.init());
  transport.onPublish=[&] {
    CHECK(mqtt.isConnected());
    for(unsigned i=0;i<200;++i) mqtt.offer({0.1f,0.2f,3990,2,i+1});
    fake_millis=4000; fake_acquisition_ms=4000;
  };
  mqtt.offer({0.1f,0.2f,1000,2,0}); mqtt.loop();
  transport.onPublish={};
  fake_millis=4499; fake_acquisition_ms=4499; mqtt.loop(); CHECK(transport.publications.size()==1);
  fake_millis=4500; fake_acquisition_ms=4500; mqtt.loop(); CHECK(transport.publications.size()==2);
  CHECK(transport.publications.back().find("\"sequence\":200")!=std::string::npos);
}

TEST(mqtt_clears_stopped_sessions_rejects_stale_values_and_observes_disconnect) {
  transport={}; fake_millis=1000; fake_acquisition_ms=1000;
  WiFiClientSecure tls; MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","",""); CHECK(mqtt.init());
  mqtt.offer({1,2,1000,1,1}); mqtt.clearPending(); mqtt.loop(); CHECK(transport.publications.empty());
  mqtt.offer({1,2,1,1,2}); fake_millis=2000; fake_acquisition_ms=2000; mqtt.loop(); CHECK(transport.publications.empty());
  Serial.output.clear(); mqtt.loop(false); CHECK(!mqtt.isConnected());
  CHECK(Serial.output.find("AWS IoT disconnected")!=std::string::npos);
  Serial.output.clear(); mqtt.loop(false); CHECK(Serial.output.empty());
  mqtt.offer({1,2,2000,2,1}); mqtt.loop(false); CHECK(transport.publications.empty());
}

TEST(mqtt_late_connection_reads_latest_mailbox_and_handles_clock_wrap) {
  transport={}; fake_millis=UINT32_MAX-100; fake_acquisition_ms=4294967195ULL;
  WiFiClientSecure tls; MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","",""); CHECK(mqtt.init());
  mqtt.offer({1,2,1,1,1});
  transport.onConnect=[&] { mqtt.offer({0.6f,-0.1f,fake_acquisition_ms,3,50}); };
  mqtt.loop(); transport.onConnect={}; CHECK(transport.publications.size()==1);
  CHECK(transport.publications[0].find("\"sequence\":50")!=std::string::npos);
  fake_millis=399; fake_acquisition_ms+=500; mqtt.offer({1,2,fake_acquisition_ms,3,51}); mqtt.loop();
  CHECK(transport.publications.size()==2);
}
