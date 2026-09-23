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

TEST(every_truncated_header_is_rejected) {
  const std::string header="Timestamp(ms),CH0(MPa),CH1(MPa)\r\n";
  for (size_t limit=0; limit<header.size(); ++limit) {
    disk={}; disk.capacity=limit;
    SDManager sd; CHECK(sd.init());
    CHECK(!sd.startRecording()); CHECK(!sd.isRecording()); CHECK(sd.hasWriteError());
  }
}

TEST(every_truncated_data_row_is_rejected_including_missing_newline) {
  const std::string row="10,0.6000,-0.1000\r\n";
  for (size_t limit=0; limit<row.size(); ++limit) {
    disk={}; fake_millis=100;
    SDManager sd; CHECK(sd.init()); CHECK(sd.startRecording());
    disk.capacity=limit; fake_millis=110;
    CHECK(!sd.logData(0.6f,-0.1f)); CHECK(sd.hasWriteError()); CHECK(!sd.isRecording());
    auto contents=disk.files.begin()->second;
    CHECK(!sd.logData(0.6f,-0.1f)); CHECK(disk.files.begin()->second==contents);
  }
}

TEST(write_error_requires_new_session_and_preserves_failed_file) {
  disk={}; fake_millis=100;
  SDManager sd; CHECK(sd.init()); CHECK(sd.startRecording());
  disk.capacity=3; CHECK(!sd.logData(1,2));
  auto failed=disk.files.begin()->second;
  disk.capacity=1000;
  CHECK(sd.startRecording()); CHECK(!sd.hasWriteError());
  CHECK(disk.files.at("/pressure_log_100.csv")==failed);
  CHECK(disk.files.count("/pressure_log_100_0001.csv"));
  CHECK(sd.logData(3,4));
}

TEST(reported_flush_error_is_not_success) {
  disk={}; SDManager sd; CHECK(sd.init());
  disk.flush_error=true; CHECK(!sd.startRecording());
  disk.flush_error=false; CHECK(sd.startRecording());
  disk.flush_error=true; CHECK(!sd.logData(1,2)); CHECK(sd.hasWriteError());
}

#include "TimeManager.h"
TEST(session_comment_short_write_is_rejected) {
  disk={}; fake_time_valid=true; WiFi.state=WL_CONNECTED;
  TimeManager time; CHECK(time.init());
  SDManager sd; sd.setTimeManager(&time); CHECK(sd.init());
  disk.capacity=std::string("Timestamp(ms),CH0(MPa),CH1(MPa)\r\n").size()+5;
  CHECK(!sd.startRecording()); CHECK(sd.hasWriteError());
  fake_time_valid=false;
}
