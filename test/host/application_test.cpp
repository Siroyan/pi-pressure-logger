#include "Test.h"
#include "LoggerApplication.h"
#include "PubSubClient.h"
#include "WiFi.h"
#include <freertos/task.h>

static void resetAppFakes() {
  disk={}; M5={}; Wire={}; WiFi={}; transport={}; task_calls=task_fail_at=0; task_names.clear();
  queue_fail_after=-1; semaphore_fail=false; fake_time_valid=false;
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

TEST(application_resource_failure_blocks_rec_and_keeps_buttons_usable) {
  for(unsigned fail=1;fail<=2;++fail) {
    resetAppFakes(); task_fail_at=fail;
    LoggerApplication app; app.setup(); app.sample(); pressA(app); CHECK(app.currentState()==STANDBY);
    M5.BtnB.pressed=true; tick(app); CHECK(app.currentState()==FILE_LIST);
    M5.BtnC.pressed=true; tick(app); CHECK(app.currentState()==STANDBY);
  }
}
