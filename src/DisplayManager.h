#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Stack.h>
#include <vector>
#include "GraphHistory.h"

class DisplayManager {
private:
  float last_displayed_v0;
  float last_displayed_v1;
  bool pressure_text_valid = false;
  GraphHistory graph;
  void drawGraphChanges();
  
  // File list display variables
  int selected_file_index;
  int scroll_offset;
  const int max_files_per_page = 8;
  
public:
  DisplayManager();
  
  void init();
  void drawLabels();
  void drawSample(float p0, float p1, uint64_t acquired_at);
  void advanceGraph(uint64_t now);
  void drawPressureText(float p0, float p1);
  void drawConnectionStatus(bool adc_available, bool sd_available, bool sd_recording,
                            bool sd_error,
                            bool wifi_connected, bool mqtt_connected);
  
  // File list display functions
  void drawFileList(const std::vector<String>& files, const std::vector<long>& fileSizes);
  void navigateFileList(int direction, int total_files);
  int getSelectedFileIndex() const;
  void resetFileListNavigation();
  void drawButtonInstructions(const String& instructions = "");
};

#endif // DISPLAY_MANAGER_H
