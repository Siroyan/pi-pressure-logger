#include "Test.h"
#include "StorageService.h"

TEST(stalled_storage_keeps_buttons_display_queue_and_network_independent) {
  disk={}; fake_millis=0; fake_acquisition_ms=0;
  RecordingQueue queue; CHECK(queue.init(8));
  SDManager sd; CHECK(sd.init()); StorageService service(sd,queue); CHECK(service.init());
  CHECK(queue.start()); service.step();
  for (unsigned i=0;i<5;++i) { fake_acquisition_ms+=10; CHECK(queue.submit(0.1f,0.2f)); }
  service.step();
  // Inject a stall inside SD.write; consumers/controls must not require its lock.
  bool exercised=false;
  disk.onWrite=[&] {
    if (exercised) return;
    exercised=true;
    CHECK(!service.tryBeginDisplay()); // Shared SPI stays busy; UI skips rendering.
    for (unsigned i=0;i<20;++i) { fake_acquisition_ms+=10; queue.submit(0.3f,0.4f); }
    RecordEvent network;
    CHECK(queue.receiveNetwork(network));
    CHECK(network.kind==RecordKind::Sample && network.sample.timestamp==250);
    CHECK(queue.stop()); // A-button stop is accepted while SD is blocked.
    PressureSample display; CHECK(queue.receiveDisplay(display));
  };
  fake_millis=1000; service.step();
  CHECK(exercised);
  CHECK(queue.droppedSamples()==14);
  disk.onWrite={}; service.step();
  CHECK(!sd.isRecording());
  CHECK(disk.files.begin()->second.find("dropped: 14")!=std::string::npos);
  CHECK(service.tryBeginDisplay()); service.endDisplay();
}

TEST(recording_holds_one_file_and_flushes_at_deadline_and_stop) {
  disk={}; fake_millis=100;
  SDManager sd; CHECK(sd.init()); CHECK(sd.startRecording(100));
  CHECK(disk.opens==1); CHECK(disk.closes==0);
  for (unsigned i=0;i<100;++i) CHECK(sd.logData({0.1f,0.2f,100+i*10}));
  CHECK(disk.opens==1); CHECK(disk.closes==0);
  fake_millis=1100; sd.poll();
  CHECK(disk.files.begin()->second.find("990,0.1000,0.2000")!=std::string::npos);
  CHECK(sd.logData({0.3f,0.4f,1100})); sd.stopRecording();
  CHECK(disk.closes==1);
  CHECK(disk.files.begin()->second.find("1000,0.3000,0.4000")!=std::string::npos);
}

TEST(file_operations_are_queued_and_run_only_after_recording_drains) {
  disk={}; fake_millis=50;
  SDManager sd; CHECK(sd.init()); RecordingQueue queue; CHECK(queue.init(16));
  StorageService service(sd,queue); CHECK(service.init());
  CHECK(queue.start()); CHECK(queue.submit(1,2));
  CHECK(service.requestList()); CHECK(disk.opens==0);
  service.step();
  std::vector<String> files; std::vector<long> sizes; bool success;
  CHECK(!service.takeResult(files,sizes,success));
  CHECK(queue.stop()); service.step();
  CHECK(service.takeResult(files,sizes,success)); CHECK(success && files.size()==1);
  CHECK(service.requestDelete(files[0])); CHECK(disk.files.size()==1);
  service.step(); CHECK(service.takeResult(files,sizes,success));
  CHECK(success && files.empty() && disk.files.empty());
}
