#pragma once
#include <Arduino.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>

inline unsigned logNameGroup(const String& name) {
  const char* p=name.c_str();
  if (name.startsWith("pressure_log_boot_")) return 1;
  if (name.length()>17 && p[17]=='-') return 2; // 日時を含むファイル名。
  return 0; // 旧形式の稼働時間ベースの名前では、起動をまたぐ日時を比較できない。
}

inline bool logNameNewer(const String& a, const String& b) {
  unsigned ga=logNameGroup(a), gb=logNameGroup(b);
  if (ga!=gb) return ga>gb;
  const char* x=a.c_str(); const char* y=b.c_str();
  while (*x && *y) {
    if (*x>='0' && *x<='9' && *y>='0' && *y<='9') {
      const char* xe=x; const char* ye=y;
      while (*xe>='0' && *xe<='9') ++xe;
      while (*ye>='0' && *ye<='9') ++ye;
      while (x<xe && *x=='0') ++x;
      while (y<ye && *y=='0') ++y;
      if (xe-x!=ye-y) return xe-x>ye-y;
      int cmp=strncmp(x,y,xe-x);
      if (cmp) return cmp>0;
      x=xe; y=ye;
    } else {
      if (*x!=*y) return *x>*y;
      ++x; ++y;
    }
  }
  if (*x!=*y) return *x>*y;
  return a>b; // 数値が同じでもゼロ埋めが違う場合は、文字列で順序を確定する。
}

inline uint64_t logBootNumber(const String& name) {
  const char* prefix="pressure_log_boot_";
  if (!name.startsWith(prefix)) return 0;
  const char* start=name.c_str()+strlen(prefix);
  if (*start<'0' || *start>'9') return 0;
  errno=0; char* end=nullptr;
  auto value=strtoull(start,&end,10);
  return errno==0 && end && *end=='_' ? value : 0;
}
