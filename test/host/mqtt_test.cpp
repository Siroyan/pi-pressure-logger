#include "Test.h"
#include "MQTTManager.h"

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
  mqtt.loop(false); CHECK(!mqtt.isConnected());
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
