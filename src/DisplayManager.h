#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Stack.h>
#include <vector>
#include "GraphHistory.h"
#include "FileAction.h"

class DisplayManager {
private:
  float last_displayed_v0;
  float last_displayed_v1;
  bool pressure_text_valid = false;
  GraphHistory graph;
  void drawGraphChanges();
  
  int selected_file_index;
  int scroll_offset;
  const int max_files_per_page = 8;
  
public:
  DisplayManager();
  
  void init();
  void drawLabels();
  // 圧力はMPa、acquired_atは起動後の単調増加ミリ秒。範囲外の値は描画時だけ切り詰める。
  void drawSample(float p0, float p1, uint64_t acquired_at);
  // サンプルが届かなくても、現在時刻に合わせて古い描画を消す。
  void advanceGraph(uint64_t now);
  void drawPressureText(float p0, float p1);
  void drawConnectionStatus(bool adc_available, bool sd_available, bool sd_recording,
                            bool sd_error,
                            bool wifi_connected, bool mqtt_connected, bool network_enabled = true);
  
  void drawFileList(const std::vector<String>& files, const std::vector<long>& fileSizes);
  void drawFileAction(const FileAction& action);
  void navigateFileList(int direction, int total_files);
  int getSelectedFileIndex() const;
  void resetFileListNavigation();
  void drawButtonInstructions(const String& instructions = "");
};

#endif
