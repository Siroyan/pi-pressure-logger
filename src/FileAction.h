#pragma once
#include <Arduino.h>

class FileAction {
public:
  enum class Status { None, Confirm, Busy, Success, Failure };
private:
  Status state = Status::None;
  String selected;
  long selectedSize = -1;
public:
  Status status() const { return state; }
  const String& filename() const { return selected; }
  long size() const { return selectedSize; }
  bool begin(const String& name, long bytes) {
    if (state != Status::None || name.length()==0) return false;
    selected=name; selectedSize=bytes; state=Status::Confirm; return true;
  }
  bool confirm() {
    if (state != Status::Confirm) return false;
    state=Status::Busy; return true;
  }
  void complete(bool success) { if (state==Status::Busy) state=success ? Status::Success : Status::Failure; }
  bool dismiss() {
    if (state==Status::Busy) return false;
    state=Status::None; selected=""; selectedSize=-1; return true;
  }
};
