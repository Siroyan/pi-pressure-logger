#include "Test.h"
#include "RecordingWriter.h"
#include <deque>

struct EventBuffer {
  std::deque<RecordEvent> events;
  size_t capacity=512;
  bool operator()(const RecordEvent& event, unsigned reserve) {
    if (capacity-events.size() <= reserve) return false;
    events.push_back(event); return true;
  }
};

TEST(delayed_start_stop_restart_preserves_session_boundaries) {
  disk={}; fake_millis=100;
  SDManager sd; CHECK(sd.init()); RecordingWriter writer(sd);
  RecordingSession session; EventBuffer queue;
  CHECK(session.sample({9,9,10},queue));
  CHECK(session.start(20,queue));
  CHECK(session.sample({1,2,21},queue));
  CHECK(session.sample({3,4,22},queue));
  CHECK(session.stop(23,queue));
  CHECK(session.sample({8,8,24},queue));
  CHECK(session.start(30,queue));
  CHECK(session.sample({5,6,31},queue));
  CHECK(session.stop(32,queue));
  unsigned published=0;
  for (const auto& event:queue.events) published+=writer.process(event);
  CHECK(published==3);
  CHECK(disk.files.size()==2);
  const auto& first=disk.files.at("/pressure_log_boot_0000000001_00000000000000000020.csv");
  const auto& second=disk.files.at("/pressure_log_boot_0000000001_00000000000000000030.csv");
  CHECK(first.find("1,1.0000,2.0000\r\n2,3.0000,4.0000\r\n")!=first.npos);
  CHECK(second.find("1,5.0000,6.0000\r\n")!=second.npos);
  CHECK(first.find("9.0000")==first.npos); CHECK(second.find("8.0000")==second.npos);
  CHECK(!sd.isRecording());
}

TEST(full_sample_queue_always_accepts_stop_and_rejects_unaccepted_start) {
  RecordingSession session; EventBuffer queue; queue.capacity=5;
  CHECK(session.start(0,queue));
  CHECK(session.sample({1,2,1},queue));
  CHECK(session.sample({1,2,2},queue));
  CHECK(!session.sample({1,2,3},queue));
  CHECK(session.stop(4,queue));
  CHECK(queue.events.back().kind==RecordKind::Stop);
  CHECK(!session.start(5,queue));
  queue.events.clear();
  CHECK(session.sample({1,2,6},queue));
  CHECK(queue.events.back().session==0);
  CHECK(session.start(7,queue));
  CHECK(queue.events.back().session==2);
}

TEST(sd_failure_does_not_reclassify_network_session) {
  disk={}; disk.capacity=2;
  SDManager sd; CHECK(sd.init()); RecordingWriter writer(sd);
  writer.process({RecordKind::Start,{0,0,10},1});
  CHECK(sd.hasWriteError());
  CHECK(writer.process({RecordKind::Sample,{0.1f,0.2f,11},1}));
  writer.process({RecordKind::Stop,{0,0,12},1});
  CHECK(!writer.process({RecordKind::Sample,{0.1f,0.2f,13},1}));
}
