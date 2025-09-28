#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <M5Stack.h>
#include <vector>

class DisplayManager {
private:
  float last_displayed_v0;
  float last_displayed_v1;
  
  // File list display variables
  int selected_file_index;
  int scroll_offset;
  const int max_files_per_page = 8;
  
public:
  DisplayManager();
  
  void init();
  void drawLabels();
  void drawOnePoint(int i, float p0, float p1, const float* ch0_buffer, const float* ch1_buffer, int buffer_size);
  void drawPressureText(float p0, float p1);
  void drawConnectionStatus(bool sd_available, bool sd_recording, bool wifi_connected, bool mqtt_connected);
  
  // File list display functions
  void drawFileList(const std::vector<String>& files, const std::vector<long>& fileSizes);
  void navigateFileList(int direction, int total_files);
  int getSelectedFileIndex() const;
  void resetFileListNavigation();
  void drawButtonInstructions(const String& instructions = "");
};

#endif // DISPLAY_MANAGER_H