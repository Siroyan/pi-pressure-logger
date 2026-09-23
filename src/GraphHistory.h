#pragma once
#include <algorithm>
#include <cstdint>

class GraphHistory {
public:
  // 138 disjoint columns of two pixels; frame stays at x=30..309.
  static constexpr unsigned columns=138;
  static constexpr uint64_t windowMs=20000;
  static constexpr unsigned gapColumns=7; // ceil(1000ms * 138 / 20000ms)
  struct Column {
    bool valid=false, dirty=false;
    uint64_t tick=0;
    float low[2]={0,0}, high[2]={0,0};
  };
private:
  Column data[columns];
  bool initialized=false;
  uint64_t head=0;
  static uint64_t bucket(uint64_t ms) {
    return (ms/windowMs)*columns + (ms%windowMs)*columns/windowMs;
  }
public:
  void reset() { for (auto& c:data) c=Column{}; initialized=false; head=0; }
  void advance(uint64_t ms) {
    uint64_t tick=bucket(ms);
    if (initialized && tick<=head) return;
    head=tick; initialized=true;
    for (auto& c:data) {
      if (c.valid && head-c.tick>=columns-gapColumns) { c.valid=false; c.dirty=true; }
    }
  }
  void add(float p0,float p1,uint64_t ms) {
    advance(ms);
    uint64_t tick=bucket(ms);
    if (head-tick>=columns-gapColumns) return; // Too old to be in the visible window.
    Column& c=data[tick%columns];
    if (!c.valid || c.tick!=tick) {
      c.valid=c.dirty=true; c.tick=tick;
      c.low[0]=c.high[0]=p0; c.low[1]=c.high[1]=p1;
    } else {
      float values[]={p0,p1};
      for (unsigned ch=0;ch<2;++ch) {
        if (values[ch]<c.low[ch] || values[ch]>c.high[ch]) c.dirty=true;
        c.low[ch]=(std::min)(c.low[ch],values[ch]);
        c.high[ch]=(std::max)(c.high[ch],values[ch]);
      }
    }
  }
  const Column& column(unsigned i) const { return data[i]; }
  void painted(unsigned i) { data[i].dirty=false; }
};
