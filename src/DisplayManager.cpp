#include "DisplayManager.h"

DisplayManager::DisplayManager() : last_displayed_v0(-1.0), last_displayed_v1(-1.0), 
  selected_file_index(0), scroll_offset(0) {}

void DisplayManager::init() {
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);
  drawLabels();
}

void DisplayManager::drawLabels() {
  M5.Lcd.setCursor(30, 10);   M5.Lcd.print("CH0");
  M5.Lcd.setCursor(30, 110);  M5.Lcd.print("CH1");

  M5.Lcd.drawRect(30, 20, 280, 80, WHITE);   // CH0
  M5.Lcd.drawRect(30, 120, 280, 80, WHITE);  // CH1
  
  // Scale marks and legends on left border
  M5.Lcd.setTextColor(0x7BEF);  // Gray color
  
  // CH0 scale marks (0.0MPa, 0.25MPa, 0.5MPa, 0.75MPa, 1.0MPa)
  for (int i = 0; i <= 4; i++) {
    int y = 99 - (i * 78 / 4);  // y=99,79,59,39,20
    float pressure = i * 0.25f;  // 0.0, 0.25, 0.5, 0.75, 1.0
    M5.Lcd.drawLine(28, y, 30, y, WHITE);  // Tick mark
    M5.Lcd.setCursor(2, y - 3);
    M5.Lcd.printf("%.2f", pressure);
  }
  
  // CH1 scale marks (0.0MPa, 0.25MPa, 0.5MPa, 0.75MPa, 1.0MPa)
  for (int i = 0; i <= 4; i++) {
    int y = 199 - (i * 78 / 4);  // y=199,179,159,139,120
    float pressure = i * 0.25f;  // 0.0, 0.25, 0.5, 0.75, 1.0
    M5.Lcd.drawLine(28, y, 30, y, WHITE);  // Tick mark
    M5.Lcd.setCursor(2, y - 3);
    M5.Lcd.printf("%.2f", pressure);
  }
  
  M5.Lcd.setTextColor(WHITE);  // Reset to white
  
  // Button instructions at bottom right
  drawButtonInstructions();
}

void DisplayManager::drawOnePoint(int i, float p0, float p1, const float* ch0_buffer, const float* ch1_buffer, int buffer_size) {
  // Constrain pressure to 0.0~1.0 MPa
  p0 = constrain(p0, 0.0, 1.0);
  p1 = constrain(p1, 0.0, 1.0);

  int x = 31 + (i * 278 / buffer_size);  // x=31-308 (inside border)

  // Clear previous waveform (inside only)
  M5.Lcd.fillRect(x, 21, 1, 78, BLACK);   // CH0 (y=21-98)
  M5.Lcd.fillRect(x, 121, 1, 78, BLACK);  // CH1 (y=121-198)

  // Draw pixels (1.0MPa = top, 0.0MPa = bottom, limited to inside area)
  int y0 = 21 + 78 - (p0 / 1.0f) * 78;  // y=21-98 (inside border)
  int y1 = 121 + 78 - (p1 / 1.0f) * 78; // y=121-198 (inside border)

  M5.Lcd.drawPixel(x, y0, GREEN);
  M5.Lcd.drawPixel(x, y1, CYAN);
}

void DisplayManager::drawPressureText(float p0, float p1) {
  if (abs(p0 - last_displayed_v0) > 0.001 || abs(p1 - last_displayed_v1) > 0.001) {
    M5.Lcd.fillRect(30, 210, 250, 15, BLACK);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(30, 210);
    M5.Lcd.printf("CH0: %.3fMPa, CH1: %.3fMPa", p0, p1);
    
    last_displayed_v0 = p0;
    last_displayed_v1 = p1;
  }
}

void DisplayManager::drawConnectionStatus(bool sd_available, bool sd_recording, bool wifi_connected, bool mqtt_connected) {
  M5.Lcd.fillRect(30, 225, 150, 10, BLACK);  // Clear area
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(30, 225);
  
  // SD Status
  if (sd_available) {
    if (sd_recording) {
      M5.Lcd.setTextColor(RED);
      M5.Lcd.print("SD:REC");
    } else {
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.print("SD:OK!");
    }
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("SD:ERR");
  }
  
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print(" ");
  
  // WiFi Status
  if (wifi_connected) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.print("WiFi");
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("WiFi!");
  }
  
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print(" ");
  
  // AWS IoT Status
  if (mqtt_connected) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.print("AWS");
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("AWS!");
  }
  
  M5.Lcd.setTextColor(WHITE);
}

void DisplayManager::drawFileList(const std::vector<String>& files, const std::vector<long>& fileSizes) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(WHITE);
  
  // Title
  M5.Lcd.setCursor(10, 5);
  M5.Lcd.print("SD Card Files (" + String(files.size()) + " files)");
  
  // Instructions at bottom right
  drawButtonInstructions("A:Down B:Delete C:Back");
  
  if (files.empty()) {
    M5.Lcd.setCursor(50, 100);
    M5.Lcd.print("No log files found");
    return;
  }
  
  // Calculate visible range
  int start_index = scroll_offset;
  int max_end = start_index + max_files_per_page;
  int end_index = (max_end < (int)files.size()) ? max_end : (int)files.size();
  
  // Draw file list
  for (int i = start_index; i < end_index; i++) {
    int y = 25 + (i - start_index) * 22;
    
    // Highlight selected file
    if (i == selected_file_index) {
      M5.Lcd.fillRect(5, y - 2, 310, 18, BLUE);
      M5.Lcd.setTextColor(WHITE);
    } else {
      M5.Lcd.setTextColor(GREEN);
    }
    
    // File name (truncated if too long)
    String filename = files[i];
    if (filename.length() > 25) {
      filename = filename.substring(0, 22) + "...";
    }
    
    M5.Lcd.setCursor(10, y);
    M5.Lcd.print(filename);
    
    // File size
    if (i < fileSizes.size() && fileSizes[i] >= 0) {
      String sizeStr;
      long size = fileSizes[i];
      if (size < 1024) {
        sizeStr = String(size) + "B";
      } else if (size < 1024 * 1024) {
        sizeStr = String(size / 1024) + "KB";
      } else {
        sizeStr = String(size / (1024 * 1024)) + "MB";
      }
      
      M5.Lcd.setCursor(250, y);
      M5.Lcd.print(sizeStr);
    }
    
    M5.Lcd.setTextColor(WHITE);
  }
  
  // Scroll indicator
  if (files.size() > max_files_per_page) {
    M5.Lcd.setCursor(290, 200);
    M5.Lcd.print(String(selected_file_index + 1) + "/" + String(files.size()));
  }
}

void DisplayManager::navigateFileList(int direction, int total_files) {
  if (total_files == 0) return;
  
  selected_file_index += direction;
  
  // Wrap around
  if (selected_file_index < 0) {
    selected_file_index = total_files - 1;
  } else if (selected_file_index >= total_files) {
    selected_file_index = 0;
  }
  
  // Update scroll offset
  if (selected_file_index < scroll_offset) {
    scroll_offset = selected_file_index;
  } else if (selected_file_index >= scroll_offset + max_files_per_page) {
    scroll_offset = selected_file_index - max_files_per_page + 1;
  }
}

int DisplayManager::getSelectedFileIndex() const {
  return selected_file_index;
}

void DisplayManager::resetFileListNavigation() {
  selected_file_index = 0;
  scroll_offset = 0;
}

void DisplayManager::drawButtonInstructions(const String& instructions) {
  M5.Lcd.setCursor(180, 225);
  M5.Lcd.setTextColor(YELLOW);
  if (instructions.length() > 0) {
    M5.Lcd.print(instructions);
  } else {
    M5.Lcd.print("A:Record B:Files");
  }
  M5.Lcd.setTextColor(WHITE);
}