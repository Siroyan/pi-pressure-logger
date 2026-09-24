#include "DisplayManager.h"

namespace {
constexpr unsigned long kSdErrorBlinkHalfPeriodMs = 500;
constexpr int kGraphTickPixelSpan = 75;
constexpr int kGraphLinePixelSpan = 74;
constexpr int kGraphLineWidth = 2;
}

DisplayManager::DisplayManager() : last_displayed_v0(-1.0), last_displayed_v1(-1.0), 
  selected_file_index(0), scroll_offset(0) {}

void DisplayManager::init() {
  pressure_text_valid = false;
  graph.reset();
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
  
  // Scale marks for both channels (0.0 to 0.5 MPa).
  for (int channel = 0; channel < 2; channel++) {
    for (int i = 0; i <= 4; i++) {
      int y = 97 + channel * 100 - (i * kGraphTickPixelSpan / 4);
      float pressure = i * 0.125f;
      M5.Lcd.drawLine(28, y, 30, y, WHITE);
      M5.Lcd.setCursor(2, y - 3);
      M5.Lcd.printf("%.2f", pressure);
    }
  }
  
  M5.Lcd.setTextColor(WHITE);  // Reset to white
  
  // Button instructions at bottom right
  drawButtonInstructions();
}

void DisplayManager::drawSample(float p0, float p1, uint64_t acquired_at) {
  graph.add(p0,p1,acquired_at);
  drawGraphChanges();
}

void DisplayManager::advanceGraph(uint64_t now) {
  graph.advance(now);
  drawGraphChanges();
}

void DisplayManager::drawGraphChanges() {
  for (unsigned i=0; i<GraphHistory::columns; ++i) {
    const auto& column=graph.column(i);
    if (!column.dirty) continue;
    const int x=32 + i*kGraphLineWidth;
    for (unsigned ch=0; ch<2; ++ch) {
      const int top=22 + ch*100, bottom=96 + ch*100;
      M5.Lcd.fillRect(x,top,kGraphLineWidth,76,BLACK);
      if (column.valid) {
        float low=constrain(column.low[ch],0.0,0.5);
        float high=constrain(column.high[ch],0.0,0.5);
        int yTop=bottom-(high/0.5f)*kGraphLinePixelSpan;
        int yBottom=bottom-(low/0.5f)*kGraphLinePixelSpan;
        M5.Lcd.fillRect(x,yTop,kGraphLineWidth,yBottom-yTop+kGraphLineWidth,ch ? CYAN : GREEN);
      }
    }
    graph.painted(i);
  }
}

void DisplayManager::drawPressureText(float p0, float p1) {
  if (!pressure_text_valid || abs(p0 - last_displayed_v0) > 0.001 || abs(p1 - last_displayed_v1) > 0.001) {
    M5.Lcd.fillRect(30, 210, 250, 15, BLACK);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(30, 210);
    M5.Lcd.printf("CH0: %.3fMPa, CH1: %.3fMPa", p0, p1);
    
    last_displayed_v0 = p0;
    last_displayed_v1 = p1;
    pressure_text_valid = true;
  }
}

void DisplayManager::drawConnectionStatus(bool adc_available, bool sd_available, bool sd_recording,
                                          bool sd_error,
                                          bool wifi_connected, bool mqtt_connected, bool network_enabled) {
  M5.Lcd.fillRect(30, 225, 150, 10, BLACK);  // Clear area
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(30, 225);
  
  // ADC Status
  M5.Lcd.setTextColor(adc_available ? GREEN : RED);
  M5.Lcd.print(adc_available ? "ADC" : "ADC!");

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
  
  if (!network_enabled) { M5.Lcd.print("OFFLINE"); return; }

  // WiFi Status
  M5.Lcd.setTextColor(wifi_connected ? GREEN : RED);
  M5.Lcd.print(wifi_connected ? "WiFi" : "WiFi!");
  
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.print(" ");
  
  // AWS IoT Status
  M5.Lcd.setTextColor(mqtt_connected ? GREEN : RED);
  M5.Lcd.print(mqtt_connected ? "AWS" : "AWS!");
  
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
    if (filename.startsWith("pressure_log_")) filename = filename.substring(13);
    
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

void DisplayManager::drawFileAction(const FileAction& action) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1); M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(10,10);
  const auto status=action.status();
  M5.Lcd.print(status==FileAction::Status::Confirm ? "Delete this file?" :
               status==FileAction::Status::Busy ? "Deleting..." :
               status==FileAction::Status::Success ? "Deleted" : "Delete failed");
  const String& name=action.filename();
  for (unsigned offset=0; offset<name.length(); offset+=48) {
    M5.Lcd.setCursor(10,40+(offset/48)*14);
    M5.Lcd.print(name.substring(offset, (offset+48<name.length()) ? offset+48 : name.length()));
  }
  M5.Lcd.setCursor(10,100);
  M5.Lcd.print(action.size()>=0 ? String(action.size())+" bytes" : String("Size unavailable"));
  M5.Lcd.setCursor(10,225); M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.print(status==FileAction::Status::Confirm ? "B:Confirm delete  C:Cancel" :
               status==FileAction::Status::Busy ? "Please wait" : "C:Back to files");
  M5.Lcd.setTextColor(WHITE);
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
