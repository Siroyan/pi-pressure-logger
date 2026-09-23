#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <type_traits>
class String {
  std::string s;
public:
  String() = default;
  String(const char* v) : s(v) {}
  String(const std::string& v) : s(v) {}
  template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
  String(T v) : s(std::to_string(v)) {}
  String(float v, unsigned decimals) {
    char b[64]; std::snprintf(b, sizeof(b), "%.*f", int(decimals), double(v)); s = b;
  }
  const char* c_str() const { return s.c_str(); }
  unsigned length() const { return s.size(); }
  int indexOf(char c) const { auto p=s.find(c); return p==s.npos ? -1 : int(p); }
  int lastIndexOf(char c) const { auto p=s.rfind(c); return p==s.npos ? -1 : int(p); }
  String substring(unsigned start) const { return s.substr(start); }
  String substring(unsigned start, unsigned end) const { return s.substr(start, end-start); }
  bool startsWith(const String& v) const { return s.compare(0, v.length(), v.s)==0; }
  bool endsWith(const String& v) const { return s.size()>=v.s.size() && s.compare(s.size()-v.s.size(), v.s.size(), v.s)==0; }
  void replace(char a, char b) { std::replace(s.begin(),s.end(),a,b); }
  void replace(const String& a,const String& b) { size_t p=0; while ((p=s.find(a.s,p))!=s.npos) { s.replace(p,a.length(),b.s); p+=b.length(); } }
  String& operator+=(const String& b) { s+=b.s; return *this; }
  friend String operator+(const String& a,const String& b) { return a.s+b.s; }
  friend bool operator==(const String& a,const String& b) { return a.s==b.s; }
  friend bool operator!=(const String& a,const String& b) { return !(a==b); }
  friend bool operator>(const String& a,const String& b) { return a.s>b.s; }
  friend bool operator<(const String& a,const String& b) { return a.s<b.s; }
};
inline uint32_t fake_millis = 0;
inline uint32_t fake_micros = 0;
inline unsigned long micros() { return fake_micros; }
inline void delayMicroseconds(unsigned long us) { fake_micros += us; }
inline unsigned long millis() { return fake_millis; }
inline void delay(unsigned long ms) { fake_millis += ms; }
inline bool fake_time_valid = false;
inline bool getLocalTime(tm* result, uint32_t timeout = 5000) {
  if (!fake_time_valid) { fake_millis += timeout; return false; }
  *result = {}; result->tm_year=126; result->tm_mon=8; result->tm_mday=24; return true;
}
inline unsigned fake_ntp_requests = 0;
inline void configTime(long, int, const char*) { ++fake_ntp_requests; }
struct FakeSerial {
  template<class T> void print(const T&) {}
  template<class T> void println(const T&) {}
  void println() {}
  template<class... T> void printf(const char*, T...) {}
};
inline FakeSerial Serial;
