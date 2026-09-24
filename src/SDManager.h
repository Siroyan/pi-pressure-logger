#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <SD.h>
#include <FS.h>
#include <vector>
#include <atomic>
#include "PressureSample.h"

class TimeManager;

class SDManager {
private:
  std::atomic<bool> sd_available;
  String log_filename;
  uint64_t session_start_time;
  std::atomic<bool> recording;
  std::atomic<bool> write_error;
  File log_file;
  char batch[1024];
  size_t batch_size = 0;
  uint32_t last_flush = 0;
  TimeManager* timeManager;
  struct LogEntry { String name; long size; };
  std::vector<LogEntry> file_cache;
  bool cache_valid = false;
  uint64_t boot_number = 0;
  bool refreshFileCache();
  String uptimeFilename();
  
public:
  SDManager();
  
  void setTimeManager(TimeManager* tm);
  bool init();
  bool isAvailable();
  bool isRecording();
  bool hasWriteError();
  
  // started_atは起動後の単調増加ミリ秒。成功時は新しいCSVを開き、既存ファイルには追記しない。
  // 戻り値はファイル作成の成否であり、後続のデータが保存済みであることは示さない。
  bool startRecording(uint64_t started_at);
  // 残りのバッファを書き出してファイルを閉じる。書き込みエラーはhasWriteError()で確認する。
  void stopRecording();
  // タイムスタンプを録画開始時からのミリ秒に変換してバッファへ追加する。
  // trueはバッファへの追加成功であり、SDへの書き込み完了ではない。
  bool logData(const PressureSample& sample);
  // 録画中のバッファをSDへ書き出す。書き込みエラー時は録画を停止してエラーを残す。
  bool flush();
  void poll();
  void writeSummary(uint32_t session, uint32_t dropped, uint32_t high_water);
  
  // 一覧取得に失敗した場合も空の一覧を返す。
  std::vector<String> getLogFileList();
  // filenameは先頭の「/」を含まない一覧上の名前。削除成功時だけキャッシュから除く。
  bool deleteFile(const String& filename);
  // ファイルが見つからない場合や一覧取得に失敗した場合は-1を返す。
  long getFileSize(const String& filename);
  
private:
  String createLogFile();
  String createUniqueFilename(const String& filename);
  bool writeLine(File& file, const String& line);
  void failWrite();
  bool append(const String& bytes);
  bool writeBatch();
};

#endif
