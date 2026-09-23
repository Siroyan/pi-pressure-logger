#include "Test.h"
#include "SDManager.h"

TEST(sd_recovery_reuses_m5stack_card_chip_select_instead_of_variant_default) {
  disk={}; SDManager sd; CHECK(sd.init()); CHECK(sd.startRecording(0));
  disk.capacity=0; CHECK(sd.logData({1,2,10})); CHECK(!sd.flush());
  disk.capacity=1024; CHECK(sd.startRecording(20));
  CHECK(disk.mountPins.size()==2 && disk.mountPins[0]==4 && disk.mountPins[1]==4);
}
