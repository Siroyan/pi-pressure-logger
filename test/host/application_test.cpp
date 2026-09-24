#include "Test.h"
#include "LoggerApplication.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include <freertos/task.h>

static void resetAppFakes() {
  disk={}; M5={}; Wire={}; WiFi={}; transport={}; task_calls=task_fail_at=0; task_names.clear();
  queue_fail_after=-1; semaphore_fail=false; semaphore_fail_after=-1; fake_time_valid=false;
  Serial.output.clear();
  fake_millis=100; fake_micros=100000; fake_acquisition_ms=100;
}
static void tick(LoggerApplication& app) {
  app.tick(); M5.BtnA={}; M5.BtnB={}; M5.BtnC={};
}
static void pressA(LoggerApplication& app) {
  M5.BtnA.pressed=true; tick(app); fake_millis+=60; fake_acquisition_ms+=60;
  M5.BtnA.released=true; tick(app);
}

TEST(application_buttons_record_stop_and_confirm_delete_through_real_workers) {
  resetAppFakes(); LoggerApplication app; app.setup(); app.sample(); tick(app);
  CHECK(!M5.sdAutoInit);
  pressA(app); CHECK(app.currentState()==RECORDING);
  app.sample(); app.store(); pressA(app); CHECK(app.currentState()==STANDBY); app.store();
  CHECK(disk.files.size()==1);
  M5.BtnB.pressed=true; tick(app); app.store(); tick(app);
  CHECK(app.currentState()==FILE_LIST);
  unsigned opens=disk.opens;
  M5.BtnB.pressed=true; tick(app); CHECK(disk.files.size()==1); // Confirmation only.
  unsigned clears=M5.Lcd.clears;
  tick(app); CHECK(M5.Lcd.clears==clears && disk.opens==opens); // Stable dialog does not flicker or rescan.
  M5.BtnC.pressed=true; tick(app); app.store(); tick(app); CHECK(disk.files.size()==1);
  M5.BtnB.pressed=true; tick(app); M5.BtnB.pressed=true; tick(app);
  CHECK(disk.files.size()==1); app.store(); tick(app); CHECK(disk.files.empty());
  bool success=false; for(const auto& text:M5.Lcd.text) success|=text.value=="Deleted";
  CHECK(success);
}

TEST(application_stop_button_is_serviced_while_storage_holds_shared_spi) {
  resetAppFakes(); LoggerApplication app; app.setup(); app.sample(); tick(app);
  pressA(app); app.sample(); app.store();
  bool exercised=false;
  disk.onWrite=[&] { if(exercised) return; exercised=true; pressA(app); CHECK(app.currentState()==STANDBY); };
  fake_millis+=1000; fake_acquisition_ms+=1000; app.store();
  disk.onWrite={}; app.store(); CHECK(exercised);
}

static bool displayed(const char* message) {
  for (const auto& text:M5.Lcd.text) if (text.value==message) return true;
  return false;
}

TEST(application_startup_failures_block_rec_and_keep_network_and_buttons_usable) {
  struct Failure { int queueAfter, semaphoreAfter; unsigned taskAt; const char* error; };
  const Failure failures[] = {
    {0,-1,0,"Sample queues unavailable"},
    {1,-1,0,"Sample queues unavailable"},
    {2,-1,0,"Sample queues unavailable"},
    {3,-1,0,"Storage resources unavailable"},
    {-1,0,0,"Storage resources unavailable"},
    {-1,1,0,"Storage resources unavailable"},
    {-1,-1,1,"Storage task unavailable"},
    {-1,-1,2,"Sampling task unavailable"},
  };
  for (const auto& fail:failures) {
    resetAppFakes(); queue_fail_after=fail.queueAfter;
    semaphore_fail_after=fail.semaphoreAfter; task_fail_at=fail.taskAt;
    LoggerApplication app; app.setup();
    CHECK(Serial.output.find(fail.error)!=std::string::npos);
    std::vector<std::string> expected;
    if (fail.taskAt==2) expected.push_back("pressure-storage");
#ifndef PRESSURE_OFFLINE
    expected.push_back("network-maintenance");
#endif
    CHECK(task_names==expected);
    const unsigned failedTaskCalls=fail.taskAt ? 1 : 0;
    CHECK(task_calls==expected.size()+failedTaskCalls);
    // Make ADC available so the REC rejection tests the startup guard itself.
    app.sample(); pressA(app); CHECK(app.currentState()==STANDBY);
    fake_millis=1000; tick(app); CHECK(displayed(fail.error));
    M5.BtnB.pressed=true; tick(app); CHECK(app.currentState()==FILE_LIST);
    M5.BtnC.pressed=true; tick(app); CHECK(app.currentState()==STANDBY);
  }
  resetAppFakes();
}

TEST(application_network_failure_still_allows_local_recording) {
  for (unsigned failure=0; failure<2; ++failure) {
    resetAppFakes();
    if (failure==0) transport.bufferAvailable=false;
    else task_fail_at=3;
    LoggerApplication app; app.setup();
    const std::vector<std::string> expected={"pressure-storage","pressure-sampling"};
    CHECK(task_names==expected);
#ifdef PRESSURE_OFFLINE
    CHECK(task_calls==2);
    CHECK(Serial.output.find("unavailable")==std::string::npos);
#else
    const char* error=failure==0 ? "MQTT resources unavailable" : "Network task unavailable";
    CHECK(Serial.output.find(error)!=std::string::npos);
    CHECK(task_calls==(failure==0 ? 2u : 3u));
    fake_millis=1000; tick(app); CHECK(displayed(error));
#endif
    app.sample(); pressA(app); CHECK(app.currentState()==RECORDING);
    app.sample(); app.store(); pressA(app); app.store();
    CHECK(app.currentState()==STANDBY && disk.files.size()==1);
    CHECK(disk.files.begin()->second.find("# Session:")!=std::string::npos);
  }
  resetAppFakes();
}

TEST(application_successful_setup_starts_workers_in_order_and_adc_failure_blocks_rec) {
  resetAppFakes(); Wire.nack=true;
  LoggerApplication app; app.setup();
  std::vector<std::string> expected={"pressure-storage","pressure-sampling"};
#ifndef PRESSURE_OFFLINE
  expected.push_back("network-maintenance");
#endif
  CHECK(task_names==expected && task_calls==expected.size());
  CHECK(Serial.output.find("unavailable")==std::string::npos);
  app.sample(); pressA(app); CHECK(app.currentState()==STANDBY);
  Wire.nack=false; fake_millis+=1000; app.sample();
  CHECK(app.currentState()==STANDBY);
  pressA(app); CHECK(app.currentState()==RECORDING);
}

TEST(application_reports_storage_error_first_when_network_also_fails) {
  resetAppFakes(); queue_fail_after=0; transport.bufferAvailable=false;
  LoggerApplication app; app.setup();
  CHECK(Serial.output.find("Sample queues unavailable")!=std::string::npos);
  CHECK(Serial.output.find("MQTT resources unavailable")==std::string::npos);
  fake_millis=1000; tick(app); CHECK(displayed("Sample queues unavailable"));
  resetAppFakes();
}
