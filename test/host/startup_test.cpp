#include "Test.h"
#include "RecordingQueue.h"
#include "MQTTManager.h"

TEST(mqtt_buffer_allocation_failure_is_safe_for_all_public_operations) {
  transport={}; transport.bufferAvailable=false;
  WiFiClientSecure tls;
  MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","","");
  CHECK(!mqtt.init()); CHECK(!mqtt.isReady()); CHECK(!mqtt.isConnected());
  mqtt.offer({1,2,1000,1,1}); mqtt.loop(); mqtt.clearPending();
  CHECK(transport.connects==0 && transport.publications.empty());
}

TEST(partially_allocated_recording_queues_reject_start_and_sample_safely) {
  for(int fail=0;fail<3;++fail) {
    queue_fail_after=fail;
    RecordingQueue queue; CHECK(!queue.init(8));
    CHECK(!queue.start()); CHECK(!queue.submit(1,2)); CHECK(queue.stop());
    queue_fail_after=-1;
  }
}
