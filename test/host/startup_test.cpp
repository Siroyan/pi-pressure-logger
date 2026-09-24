#include "Test.h"
#include "RuntimeStartup.h"
#include "StateManager.h"
#include "SDManager.h"
#include "MQTTManager.h"

TEST(resource_creation_failures_disable_recording_without_disabling_file_screen) {
  for(unsigned failure=0;failure<5;++failure) {
    unsigned samplingCalls=0,networkCalls=0;
    auto result=startRuntime(failure!=0,failure!=3,
      [&] { return failure!=1; }, [] { return true; }, [] { return true; },
      [&] { ++samplingCalls; return failure!=2; },
      [&] { ++networkCalls; return failure!=4; });
    CHECK(result.error!=nullptr);
    CHECK(result.acquisition==(failure>=3));
    CHECK(samplingCalls==(failure>1 ? 1u:0u));
    CHECK(networkCalls==(failure==3 ? 0u:1u));
    disk={}; SDManager sd; CHECK(sd.init());
    RecordingQueue queue; CHECK(queue.init(16));
    StateManager state(queue);
    state.setRecordingReady(result.acquisition); state.transitionToRecording();
    CHECK((state.getCurrentState()==RECORDING)==result.acquisition);
    if (!result.acquisition) { CHECK(disk.opens==0); state.transitionToFileList(); CHECK(state.getCurrentState()==FILE_LIST); }
  }
}

TEST(mqtt_buffer_allocation_failure_is_safe_for_all_public_operations) {
  transport={}; transport.bufferAvailable=false;
  WiFiClientSecure tls;
  MQTTManager mqtt(&tls,"example.invalid",8883,"test","data","","","");
  CHECK(!mqtt.init()); CHECK(!mqtt.isReady()); CHECK(!mqtt.isConnected());
  mqtt.offer({1,2,1000,1,1}); mqtt.loop(); mqtt.clearPending();
  CHECK(transport.connects==0 && transport.publications.empty());
}

TEST(storage_startup_failure_prevents_acquisition_but_allows_network_diagnostics) {
  for (unsigned fail=0; fail<4; ++fail) {
    unsigned sampling=0,storage=0;
    auto status=startRuntime(true,true,[&] { return fail!=0; },
      [&] { return fail!=1; }, [&] { ++storage; return fail!=2; },
      [&] { ++sampling; return fail!=3; }, [] { return true; });
    CHECK(!status.acquisition && status.network && status.error);
    CHECK(storage==(fail>=2 ? 1u:0u)); CHECK(sampling==(fail==3 ? 1u:0u));
  }
}

TEST(partially_allocated_recording_queues_reject_start_and_sample_safely) {
  for(int fail=0;fail<3;++fail) {
    queue_fail_after=fail;
    RecordingQueue queue; CHECK(!queue.init(8));
    CHECK(!queue.start()); CHECK(!queue.submit(1,2)); CHECK(queue.stop());
    queue_fail_after=-1;
  }
}
