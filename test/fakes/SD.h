#pragma once
#include "FS.h"
constexpr int CARD_NONE = 0;
struct FakeSD {
  bool begin() { return disk.mounted; }
  void end() {}
  int cardType() { return disk.mounted ? 1 : CARD_NONE; }
  bool exists(const char* p) { return disk.files.count(p); }
  File open(const char* p, const char* mode=FILE_READ) {
    ++disk.opens;
    if (!disk.mounted || disk.fail_open) return {};
    if (std::string(p)=="/") return File(p);
    if (std::string(mode)==FILE_WRITE) disk.files[p].clear();
    else if (std::string(mode)==FILE_READ && !exists(p)) return {};
    return File(p);
  }
  bool remove(const String& p) { return disk.files.erase(p.c_str())==1; }
};
inline FakeSD SD;
