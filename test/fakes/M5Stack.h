#pragma once
#include "Arduino.h"
#include <cmath>
#include <vector>
#include <string>
constexpr unsigned BLACK=0, WHITE=0xFFFF, GREEN=0x07E0, CYAN=0x07FF, RED=0xF800, YELLOW=0xFFE0, BLUE=0x001F;
inline float constrain(float x,double low,double high) { return std::min(std::max(double(x),low),high); }
struct FakeLcd {
  struct Rect { int x,y,w,h; unsigned color; };
  struct Text { int x,y; std::string value; };
  std::vector<Rect> rectangles;
  std::vector<Text> text;
  int x=0,y=0;
  void setRotation(int) {}
  void fillScreen(unsigned) { rectangles.clear(); text.clear(); }
  void setTextColor(unsigned) {}
  void setTextSize(unsigned) {}
  void setCursor(int a,int b) { x=a; y=b; }
  void print(const String& value) { text.push_back({x,y,value.c_str()}); }
  void drawRect(int,int,int,int,unsigned) {}
  void drawLine(int,int,int,int,unsigned) {}
  void fillRect(int a,int b,int w,int h,unsigned c) { rectangles.push_back({a,b,w,h,c}); }
  template<class... Args> void printf(const char* fmt,Args... args) {
    char value[256]; std::snprintf(value,sizeof(value),fmt,args...); print(String(value));
  }
};
struct FakeM5 { FakeLcd Lcd; };
inline FakeM5 M5;
