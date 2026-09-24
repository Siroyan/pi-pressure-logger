#include "Test.h"
#include "NetworkService.h"
#include "StorageService.h"
#include "PubSubClient.h"
#include "WiFi.h"

TEST(network_profile_can_record_without_credentials_or_connection_waits) {
  WiFi={}; transport={}; fake_time_valid=false; fake_millis=0; fake_ntp_requests=0;
  RecordingQueue queue; CHECK(queue.init(16)); NetworkService network(queue);
  network.init(); CHECK(fake_millis==0);
#ifdef PRESSURE_OFFLINE
  CHECK(!network.enabled() && network.ready() && network.timeSource()==nullptr);
  network.step(); CHECK(WiFi.begins==0 && transport.connects==0 && fake_ntp_requests==0);
  disk={}; SDManager sd; CHECK(sd.init()); StorageService storage(sd,queue); CHECK(storage.init());
  CHECK(queue.start()); CHECK(queue.submit(0.1f,0.2f)); CHECK(queue.stop()); storage.step();
  CHECK(!sd.isRecording()); CHECK(disk.files.begin()->second.find("0.1000,0.2000")!=std::string::npos);
#else
  CHECK(network.enabled() && network.ready() && network.timeSource()!=nullptr);
  CHECK(WiFi.begins==1); network.step(); CHECK(transport.connects==0 && fake_ntp_requests==0);
#endif
}
