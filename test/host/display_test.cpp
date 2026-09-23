#include "Test.h"
#include "DisplayManager.h"

TEST(unchanged_pressure_is_redrawn_after_every_screen_clear) {
  M5.Lcd={}; DisplayManager display;
  for (unsigned i=0;i<3;++i) {
    display.init();
    const auto labels=M5.Lcd.text.size();
    display.drawPressureText(0.25f,0.25f);
    CHECK(M5.Lcd.text.size()==labels+1);
    CHECK(M5.Lcd.text.back().value=="CH0: 0.250MPa, CH1: 0.250MPa");
    display.drawPressureText(0.25f,0.25f);
    CHECK(M5.Lcd.text.size()==labels+1);
  }
}
