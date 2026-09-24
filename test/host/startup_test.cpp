#include "Test.h"
#include "RuntimeStartup.h"
#include "StateManager.h"
#include "SDManager.h"
#include "MQTTManager.h"

// Legacy graph storage still owned by main, removed when ownership is refactored.
float ch0_buffer[2000]={}, ch1_buffer[2000]={};
int buf_index=0;
extern const int buffer_size=2000, graph_gap_samples=100;

TEST(resource_creation_failures_disable_recording_without_disabling_file_screen) {
  for(unsigned failure=0;failure<5;++failure) {
    unsigned samplingCalls=0,networkCalls=0;
    auto result=startRuntime(failure!=0,failure!=3,
      [&] { return failure!=1; },
      [&] { ++samplingCalls; return failure!=2; },
      [&] { ++networkCalls; return failure!=4; });
    CHECK(result.error!=nullptr);
    CHECK(result.acquisition==(failure>=3));
    CHECK(samplingCalls==(failure>1 ? 1u:0u));
    CHECK(networkCalls==(failure==3 ? 0u:1u));
    disk={}; SDManager sd; CHECK(sd.init());
    StateManager state; state.setManagers(&sd,nullptr,nullptr);
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
