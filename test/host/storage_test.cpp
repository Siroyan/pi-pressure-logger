#include "Test.h"
#include "SDManager.h"

TEST(existing_log_is_preserved_on_same_tick_restart) {
  disk={}; fake_millis=42;
  disk.files["/pressure_log_42.csv"]="existing data";
  SDManager sd;
  CHECK(sd.init()); CHECK(sd.startRecording()); sd.stopRecording();
  CHECK(sd.startRecording());
  CHECK(disk.files.at("/pressure_log_42.csv")=="existing data");
  CHECK(disk.files.count("/pressure_log_42_0001.csv"));
  CHECK(disk.files.count("/pressure_log_42_0002.csv"));
}

TEST(start_open_failure_does_not_claim_sd_recording) {
  disk={}; SDManager sd; CHECK(sd.init()); disk.fail_open=true;
  CHECK(!sd.startRecording()); CHECK(!sd.isRecording()); CHECK(sd.hasWriteError());
}

TEST(append_failure_stops_recording) {
  disk={}; SDManager sd; CHECK(sd.init()); CHECK(sd.startRecording());
  disk.fail_open=true;
  CHECK(!sd.logData(0.6f,-0.1f)); CHECK(!sd.isRecording()); CHECK(sd.hasWriteError());
}

TEST(csv_retains_out_of_display_range_values) {
  disk={}; fake_millis=100;
  SDManager sd; CHECK(sd.init()); CHECK(sd.startRecording()); fake_millis=110;
  CHECK(sd.logData(0.6f,-0.1f));
  CHECK(disk.files.begin()->second.find("10,0.6000,-0.1000\r\n")!=std::string::npos);
}
