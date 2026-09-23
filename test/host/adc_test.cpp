#include "Test.h"
#include "AcquisitionService.h"

TEST(adc_pair_preserves_conversion_gain_and_channel_selection) {
  TwoWire bus; ADCReader reader(bus); reader.init(); fake_micros=0;
  float p0=99,p1=99;
  CHECK(reader.readPair(p0,p1)); CHECK(bus.timeout==2);
  CHECK(std::abs(p0-0.05f)<0.00001f); CHECK(std::abs(p1-0.65f)<0.00001f);
  CHECK(bus.configs.size()==2 && bus.configs[0]==0xC1C3 && bus.configs[1]==0xD1C3);
  bus.counts[0]=-1; CHECK(reader.readPair(p0,p1)); CHECK(p0<-.25f);
}

TEST(adc_transfer_failure_at_any_stage_never_returns_partial_pair) {
  TwoWire good; ADCReader baseline(good); baseline.init(); float p0,p1;
  CHECK(baseline.readPair(p0,p1));
  const unsigned transfers=good.operations;
  for (unsigned fail=1;fail<=transfers;++fail) {
    TwoWire bus; bus.failAt=fail; ADCReader reader(bus); reader.init();
    p0=p1=99; CHECK(!reader.readPair(p0,p1)); CHECK(p0==99 && p1==99);
  }
}

TEST(adc_deadline_bounds_stuck_conversion_even_at_micros_wrap) {
  TwoWire bus; bus.neverReady=true; ADCReader reader(bus); reader.init();
  fake_micros=UINT32_MAX-1000; const uint32_t start=fake_micros;
  float p0=99,p1=99; CHECK(!reader.readPair(p0,p1));
  CHECK(uint32_t(fake_micros-start)<=10000); CHECK(p0==99 && p1==99);
  bus.neverReady=false; bus.badConfig=true; CHECK(!reader.readPair(p0,p1));
  bus.badConfig=false; bus.shortRead=true; CHECK(!reader.readPair(p0,p1));
  bus.shortRead=false; bus.failWrite=true; CHECK(!reader.readPair(p0,p1));
}

TEST(runtime_adc_loss_stops_session_suppresses_values_and_recovers_in_standby) {
  TwoWire bus; RecordingQueue queue; CHECK(queue.init(16));
  AcquisitionService service(queue,bus); service.init();
  fake_millis=0; service.step(); CHECK(service.isAvailable()); CHECK(queue.start());
  service.step(); bus.nack=true; service.step(); CHECK(!service.isAvailable());
  unsigned attempts=bus.operations;
  fake_millis=999; service.step(); CHECK(bus.operations==attempts);
  RecordEvent event; unsigned samples=0,stops=0;
  while(queue.receive(event)) { samples+=event.kind==RecordKind::Sample; stops+=event.kind==RecordKind::Stop; }
  CHECK(samples==1 && stops==1);
  bus.nack=false; fake_millis=1000; service.step(); CHECK(service.isAvailable());
  CHECK(!queue.receive(event)); // Recovery alone never reopens a recording.
  CHECK(queue.receiveNetwork(event)); CHECK(event.session==0);
}
