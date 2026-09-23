#include "DisplayManager.h"

namespace {
constexpr unsigned long kSdErrorBlinkHalfPeriodMs = 500;
constexpr int kGraphTickPixelSpan = 75;
constexpr int kGraphLinePixelSpan = 74;
constexpr int kGraphLineWidth = 2;
constexpr int kGraphXPixelSpan = 277;
}

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
  
  // CH0 scale marks (0.0MPa, 0.125MPa, 0.25MPa, 0.375MPa, 0.5MPa)
  for (int i = 0; i <= 4; i++) {
    int y = 97 - (i * kGraphTickPixelSpan / 4);  // y=97,79,60,41,22
    float pressure = i * 0.125f;  // 0.0, 0.125, 0.25, 0.375, 0.5
    M5.Lcd.drawLine(28, y, 30, y, WHITE);  // Tick mark
    M5.Lcd.setCursor(2, y - 3);
    M5.Lcd.printf("%.2f", pressure);
  }
  
  // CH1 scale marks (0.0MPa, 0.125MPa, 0.25MPa, 0.375MPa, 0.5MPa)
  for (int i = 0; i <= 4; i++) {
    int y = 197 - (i * kGraphTickPixelSpan / 4);  // y=197,179,160,141,122
    float pressure = i * 0.125f;  // 0.0, 0.125, 0.25, 0.375, 0.5
    M5.Lcd.drawLine(28, y, 30, y, WHITE);  // Tick mark
    M5.Lcd.setCursor(2, y - 3);
    M5.Lcd.printf("%.2f", pressure);
  }
  
  M5.Lcd.setTextColor(WHITE);  // Reset to white
  
  // Button instructions at bottom right
  drawButtonInstructions();
}

void DisplayManager::drawOnePoint(int i, float p0, float p1, const float* ch0_buffer,
                                  const float* ch1_buffer, int buffer_size, int gap_samples) {
  // Constrain pressure to 0.0~0.5 MPa
  p0 = constrain(p0, 0.0, 0.5);
  p1 = constrain(p1, 0.0, 0.5);

  int x = 31 + (i * kGraphXPixelSpan / buffer_size);  // x=31-307 (inside border)
  int gap_index = (i + gap_samples) % buffer_size;
  int gap_x = 31 + (gap_index * kGraphXPixelSpan / buffer_size);

  // Clear previous waveform (inside only)
  M5.Lcd.fillRect(x, 22, kGraphLineWidth, 76, BLACK);   // CH0 (y=22-97)
  M5.Lcd.fillRect(x, 122, kGraphLineWidth, 76, BLACK);  // CH1 (y=122-197)

  // Extend the one-second blank band ahead of the latest sample.
  M5.Lcd.fillRect(gap_x, 22, kGraphLineWidth, 76, BLACK);
  M5.Lcd.fillRect(gap_x, 122, kGraphLineWidth, 76, BLACK);

  // Draw pixels (0.5MPa = top, 0.0MPa = bottom, limited to inside area)
  int y0 = 96 - (p0 / 0.5f) * kGraphLinePixelSpan;   // y=22-96
  int y1 = 196 - (p1 / 0.5f) * kGraphLinePixelSpan;  // y=122-196

  M5.Lcd.fillRect(x, y0, kGraphLineWidth, kGraphLineWidth, GREEN);
  M5.Lcd.fillRect(x, y1, kGraphLineWidth, kGraphLineWidth, CYAN);
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

void DisplayManager::drawConnectionStatus(bool adc_available, bool sd_available, bool sd_recording,
                                          bool sd_error,
                                          bool wifi_connected, bool mqtt_connected) {
  M5.Lcd.fillRect(30, 225, 150, 10, BLACK);  // Clear area
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(30, 225);
  
  // ADC Status
  if (adc_available) {
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.print("ADC");
  } else {
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("ADC!");
  }

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print(" ");

  // SD Status
  if (sd_error) {
    bool showRed = (millis() / kSdErrorBlinkHalfPeriodMs) % 2 == 0;
    M5.Lcd.setTextColor(showRed ? RED : YELLOW);
    M5.Lcd.print("SD:ERR");
  } else if (sd_available) {
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
