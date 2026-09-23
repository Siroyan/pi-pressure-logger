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

TEST(graph_keeps_short_peaks_within_a_pixel_column) {
  GraphHistory graph;
  graph.add(0.1f,0.2f,10); graph.add(0.5f,-0.1f,20); graph.add(0.1f,0.2f,30);
  CHECK(graph.column(0).high[0]==0.5f && graph.column(0).low[0]==0.1f);
  CHECK(graph.column(0).low[1]==-0.1f);
  M5.Lcd={}; DisplayManager display; display.init();
  display.drawSample(0.1f,0.2f,10); display.drawSample(0.5f,0.2f,20); display.drawSample(0.1f,0.2f,30);
  bool peak=false;
  for (const auto& rect:M5.Lcd.rectangles) if (rect.color==GREEN && rect.y==22 && rect.h>2) peak=true;
  CHECK(peak);
}

TEST(graph_uses_elapsed_time_preserves_missing_intervals_and_moving_gap) {
  GraphHistory graph;
  graph.add(0.1f,0.2f,0); graph.add(0.1f,0.2f,1000);
  CHECK(graph.column(0).valid && graph.column(6).valid);
  for (unsigned i=1;i<6;++i) CHECK(!graph.column(i).valid);
  graph.advance(20000); CHECK(!graph.column(0).valid && !graph.column(6).valid);
  graph.add(0.3f,0.4f,20000); CHECK(graph.column(0).valid);
  for (unsigned i=1;i<=GraphHistory::gapColumns;++i) CHECK(!graph.column(i).valid);
  graph.add(9,9,0); CHECK(graph.column(0).high[0]==0.3f); // Stale backlog cannot replace a new sweep.
  graph.advance(60000);
  for(unsigned i=0;i<GraphHistory::columns;++i) CHECK(!graph.column(i).valid);
}

TEST(graph_clips_only_pixels_with_two_pixel_insets_and_width) {
  M5.Lcd={}; DisplayManager display; display.init();
  display.drawSample(-1,1,19999);
  unsigned colored=0;
  for (const auto& r:M5.Lcd.rectangles) if (r.color==GREEN || r.color==CYAN) {
    CHECK(r.w==2 && r.h==2); CHECK(r.x>=32 && r.x+r.w-1<=307);
    if(r.color==GREEN) CHECK(r.y==96); else CHECK(r.y==122);
    ++colored;
  }
  CHECK(colored==2);
}
