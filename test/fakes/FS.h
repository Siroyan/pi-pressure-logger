#pragma once
#include "Arduino.h"
#include <map>
#include <memory>
#include <limits>
#include <vector>
constexpr const char* FILE_WRITE = "w";
constexpr const char* FILE_APPEND = "a";
constexpr const char* FILE_READ = "r";
struct FakeDisk {
  std::map<std::string,std::string> files;
  bool mounted=true, fail_open=false;
  bool flush_error=false;
  size_t capacity=std::numeric_limits<size_t>::max();
  unsigned opens=0, closes=0;
};
inline FakeDisk disk;
class File {
  std::string path;
  bool open=false, directory=false;
  std::vector<std::string> names;
  size_t index=0;
public:
  File() = default;
  explicit File(std::string p) : path(std::move(p)), open(true), directory(path=="/") {
    if (directory) for (auto& f:disk.files) names.push_back(f.first);
  }
  explicit operator bool() const { return open; }
  bool isDirectory() const { return directory; }
  const char* name() const { return path.c_str()+(!path.empty() && path[0]=='/' ? 1 : 0); }
  size_t write(const uint8_t* data,size_t length) {
    if (!open) return 0;
    size_t n=std::min(length,disk.capacity); disk.files[path].append(reinterpret_cast<const char*>(data),n); disk.capacity-=n; return n;
  }
  size_t print(const String& s) { return write(reinterpret_cast<const uint8_t*>(s.c_str()),s.length()); }
  size_t println(const String& s) { size_t n=print(s); return n+print("\r\n"); }
  void flush() { error=disk.flush_error; }
  int getWriteError() const { return error; }
  void clearWriteError() {}
  void close() { if(open) ++disk.closes; open=false; }
  long size() const { auto it=disk.files.find(path); return it==disk.files.end()?0:it->second.size(); }
  File openNextFile() { return index<names.size()?File(names[index++]):File(); }
private:
  bool error=false;
};
