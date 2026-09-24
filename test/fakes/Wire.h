#pragma once
#include "Arduino.h"
#include <vector>
#include <deque>
class TwoWire {
public:
  bool nack=false, neverReady=false, badConfig=false, shortRead=false, failWrite=false;
  bool initialized=false, failBegin=false, transmitting=false;
  unsigned beginCalls=0, invalidTransfers=0;
  unsigned writes=0, failWriteAt=0;
  int sda=-1, scl=-1;
  unsigned timeout=0, operations=0, failAt=0;
  uint32_t transferUs=100;
  uint16_t config=0;
  uint8_t reg=0;
  int16_t counts[2]={200,600};
  std::vector<uint8_t> tx;
  std::deque<int> rx;
  std::vector<uint16_t> configs;
  bool begin(int dataPin, int clockPin) {
    ++beginCalls; sda=dataPin; scl=clockPin;
    initialized=!failBegin;
    return initialized;
  }
  void setTimeOut(unsigned ms) { timeout=ms; }
  void beginTransmission(uint8_t) {
    if (!initialized || transmitting) { ++invalidTransfers; return; }
    transmitting=true; tx.clear();
  }
  size_t write(uint8_t byte) {
    ++writes;
    if (!initialized || failWrite || writes==failWriteAt) return 0;
    tx.push_back(byte); return 1;
  }
  size_t write(const uint8_t* bytes,size_t size) {
    ++writes;
    if (!initialized || failWrite || writes==failWriteAt) return 0;
    tx.insert(tx.end(),bytes,bytes+size); return size;
  }
  int endTransmission(bool=true) {
    if (!initialized || !transmitting) { ++invalidTransfers; return 4; }
    transmitting=false;
    ++operations; fake_micros+=transferUs;
    if (nack || operations==failAt) return 2;
    if (!tx.empty()) reg=tx[0];
    if (tx.size()==3) { config=(uint16_t(tx[1])<<8)|tx[2]; configs.push_back(config); }
    return 0;
  }
  uint8_t requestFrom(uint8_t,uint8_t) {
    if (!initialized || transmitting) { ++invalidTransfers; return 0; }
    ++operations; fake_micros+=transferUs; rx.clear();
    if (nack || operations==failAt || shortRead) return 0;
    uint16_t value;
    if (reg==1) value=badConfig ? 0xFFFF : (neverReady ? config&0x7FFF : config|0x8000);
    else value=static_cast<uint16_t>(counts[((config>>12)&7)-4]*16);
    rx.push_back(value>>8); rx.push_back(value&255); return 2;
  }
  int read() { if (rx.empty()) return -1; int value=rx.front(); rx.pop_front(); return value; }
};
inline TwoWire Wire;
