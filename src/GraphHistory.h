#pragma once
#include <algorithm>
#include <cstdint>

class GraphHistory {
public:
  // 2画素幅の列を138本使い、外枠の位置はx=30～309のままにする。
  static constexpr unsigned columns=138;
  static constexpr uint64_t windowMs=20000;
  static constexpr unsigned gapColumns=7; // 20秒の表示範囲で約1秒分を空白にする。
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
  // msは起動後の単調増加ミリ秒。新しいサンプルがなくても古い列を無効化する。
  void advance(uint64_t ms) {
    uint64_t tick=bucket(ms);
    if (initialized && tick<=head) return;
    head=tick; initialized=true;
    for (auto& c:data) {
      if (c.valid && head-c.tick>=columns-gapColumns) { c.valid=false; c.dirty=true; }
    }
  }
  // 同じ時間列に入るサンプルの最小値と最大値を残し、短いピークを保持する。
  void add(float p0,float p1,uint64_t ms) {
    advance(ms);
    uint64_t tick=bucket(ms);
    if (head-tick>=columns-gapColumns) return; // 表示範囲と空白帯から外れた古い値は描かない。
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
