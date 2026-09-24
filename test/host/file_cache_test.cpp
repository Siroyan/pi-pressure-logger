#include "Test.h"
#include "SDManager.h"

TEST(hundreds_of_files_are_scanned_once_and_sorted_numerically) {
  disk={};
  for(unsigned i=0;i<500;++i) disk.files["/pressure_log_"+std::to_string(i)+".csv"]="row";
  disk.files["/pressure_log_1000.csv"]="four";
  disk.files["/pressure_log_1000_0001.csv"]="five!";
  disk.files["/pressure_log_2026-09-24-01-02-03.csv"]="dated";
  SDManager sd; CHECK(sd.init());
  auto files=sd.getLogFileList(); CHECK(files.size()==503);
  CHECK(files[0]=="pressure_log_2026-09-24-01-02-03.csv");
  CHECK(files[1]=="pressure_log_1000_0001.csv" && files[2]=="pressure_log_1000.csv");
  unsigned opens=disk.opens;
  for(unsigned repeat=0;repeat<10;++repeat) {
    for(const auto& name:sd.getLogFileList()) CHECK(sd.getFileSize(name)>=3);
  }
  CHECK(disk.opens==opens);
  CHECK(sd.deleteFile(files[1])); CHECK(sd.getLogFileList().size()==502);
  CHECK(disk.opens==opens);
}

TEST(unsynchronized_logs_identify_reboots_and_remount_does_not_change_boot_group) {
  disk={}; fake_time_valid=false;
  { SDManager first; CHECK(first.init()); CHECK(first.startRecording(900)); first.stopRecording();
    CHECK(first.startRecording(1000)); first.stopRecording(); }
  SDManager second; CHECK(second.init()); CHECK(second.startRecording(1)); second.stopRecording();
  auto files=second.getLogFileList(); CHECK(files.size()==3);
  CHECK(files[0]=="pressure_log_boot_0000000002_00000000000000000001.csv");
  CHECK(files[1]=="pressure_log_boot_0000000001_00000000000000001000.csv");
  CHECK(second.init()); CHECK(second.startRecording(2)); second.stopRecording();
  CHECK(second.getLogFileList()[0]=="pressure_log_boot_0000000002_00000000000000000002.csv");
}

TEST(failed_delete_retains_cached_entry_and_reports_failure) {
  disk={}; disk.files["/pressure_log_900.csv"]="content";
  SDManager sd; CHECK(sd.init()); CHECK(sd.getLogFileList().size()==1);
  disk.fail_remove=true; CHECK(!sd.deleteFile("pressure_log_900.csv"));
  CHECK(sd.getLogFileList().size()==1 && sd.getFileSize("pressure_log_900.csv")==7);
}
