#include <M5Stack.h>
#include <Adafruit_ADS1X15.h>
#include "../secure/aws_certificates.h"
#include "../secure/config.h"
#include "StateManager.h"
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "SDManager.h"
#include "DisplayManager.h"
#include "TimeManager.h"

Adafruit_ADS1015 ads;
// Voltage divider: 5V -> 2.5V (R1=47k, R2=47k)
// ADS1015 with GAIN_TWOTHIRDS: 0-6.144V range, LSB = 3mV
// Pressure conversion: 1V=0MPa, 5V=1MPa

const int duration_sec = 20;
const int sampling_rate = 100;
const int buffer_size = duration_sec * sampling_rate;

float ch0_buffer[buffer_size] = {0};
float ch1_buffer[buffer_size] = {0};
int buf_index = 0;

unsigned long last_sample_time = 0;
const int interval_ms = 1000 / sampling_rate;

// Button debounce variables
unsigned long button_press_time = 0;
const int min_press_duration = 50; // Minimum press duration in ms to be considered valid

// Manager instances
WiFiClientSecure wifiClientSecure;
WiFiManager wifiManager(ssid, password);
MQTTManager mqttManager(&wifiClientSecure, aws_iot_endpoint, aws_iot_port, thing_name, aws_root_ca, device_cert, device_key);
SDManager sdManager;
DisplayManager displayManager;
TimeManager timeManager;
StateManager stateManager;

void handleButtonInput() {
  // Handle Button A for recording toggle with debounce
  if (M5.BtnA.wasPressed()) {
    button_press_time = millis();
  }
  
  if (M5.BtnA.wasReleased() && button_press_time > 0) {
    unsigned long press_duration = millis() - button_press_time;
    if (press_duration >= min_press_duration) {
      Serial.println("BtnA valid press detected (" + String(press_duration) + "ms)");
      
      if (stateManager.getCurrentState() == FILE_LIST) {
        // In file list: Button A scrolls down
        std::vector<String> files = sdManager.getLogFileList();
        displayManager.navigateFileList(1, files.size());
        
        // Update display with file sizes
        std::vector<long> fileSizes;
        for (const String& file : files) {
          fileSizes.push_back(sdManager.getFileSize(file));
        }
        displayManager.drawFileList(files, fileSizes);
      } else {
        // Normal toggle behavior
        stateManager.toggleState();
        displayManager.drawConnectionStatus(sdManager.isAvailable(), sdManager.isRecording(), 
                                            wifiManager.isConnected(), mqttManager.isConnected());
      }
    } else {
      Serial.println("BtnA press too short (" + String(press_duration) + "ms) - ignored");
    }
    button_press_time = 0;
  }
  
  // Handle Button B
  if (M5.BtnB.wasPressed()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      // In file list: Button B deletes selected file
      std::vector<String> files = sdManager.getLogFileList();
      if (!files.empty()) {
        int selectedIndex = displayManager.getSelectedFileIndex();
        if (selectedIndex >= 0 && selectedIndex < files.size()) {
          String selectedFile = files[selectedIndex];
          
          // Confirm and delete
          if (sdManager.deleteFile(selectedFile)) {
            Serial.println("File deleted: " + selectedFile);
            
            // Refresh file list
            files = sdManager.getLogFileList();
            
            // Adjust selection if needed
            if (selectedIndex >= files.size() && files.size() > 0) {
              displayManager.navigateFileList(-1, files.size());
            }
            
            // Update display
            std::vector<long> fileSizes;
            for (const String& file : files) {
              fileSizes.push_back(sdManager.getFileSize(file));
            }
            displayManager.drawFileList(files, fileSizes);
          }
        }
      }
    } else {
      // Handle transition to file list
      stateManager.handleButtonB();
      
      if (stateManager.getCurrentState() == FILE_LIST) {
        // Entered file list mode - display files
        displayManager.resetFileListNavigation();
        std::vector<String> files = sdManager.getLogFileList();
        
        // Get file sizes
        std::vector<long> fileSizes;
        for (const String& file : files) {
          fileSizes.push_back(sdManager.getFileSize(file));
        }
        
        displayManager.drawFileList(files, fileSizes);
      } else {
        // Exited file list mode - display will be restored by StateManager
        displayManager.drawConnectionStatus(sdManager.isAvailable(), sdManager.isRecording(), 
                                            wifiManager.isConnected(), mqttManager.isConnected());
      }
    }
  }
  
  // Handle Button C for returning to waveform screen
  if (M5.BtnC.wasPressed()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      // Return to STANDBY (waveform screen)
      stateManager.transitionToStandby();
      displayManager.drawConnectionStatus(sdManager.isAvailable(), sdManager.isRecording(), 
                                          wifiManager.isConnected(), mqttManager.isConnected());
    }
  }
}

void handleNetworkMaintenance() {
  // Check WiFi connection and maintain MQTT
  wifiManager.checkConnection();
  mqttManager.loop();
}

void setup() {
  M5.begin();
  
  // Initialize sensor
  ads.begin();
  ads.setDataRate(RATE_ADS1015_3300SPS);
  ads.setGain(GAIN_TWOTHIRDS); // 0-6.144V range for 0-2.5V input

  // Initialize managers
  displayManager.init();
  sdManager.init();
  wifiManager.init();
  
  // Initialize time synchronization after WiFi
  timeManager.init();
  
  mqttManager.init();
  
  // Connect state manager to other managers
  stateManager.setManagers(&sdManager, &mqttManager, &displayManager, &timeManager);
  
  // Draw initial status
  displayManager.drawConnectionStatus(sdManager.isAvailable(), sdManager.isRecording(), 
                                      wifiManager.isConnected(), mqttManager.isConnected());
}

void loop() {
  M5.update();
  unsigned long now = millis();
  
  // Handle user input
  handleButtonInput();
  
  // Handle network maintenance
  handleNetworkMaintenance();
  
  // Update connection status display periodically
  static unsigned long last_status_update = 0;
  if (now - last_status_update >= 2000) {
    last_status_update = now;
    displayManager.drawConnectionStatus(sdManager.isAvailable(), sdManager.isRecording(), 
                                        wifiManager.isConnected(), mqttManager.isConnected());
  }
  
  // Re-synchronize time periodically (every 30 minutes)
  static unsigned long last_time_sync = 0;
  if (wifiManager.isConnected() && (now - last_time_sync >= 30 * 60 * 1000)) {
    last_time_sync = now;
    if (!timeManager.isTimeSynced()) {
      Serial.println("Re-synchronizing time...");
      timeManager.syncTime();
    }
  }
  
  // Process sensor data at regular intervals
  if (now - last_sample_time >= interval_ms) {
    last_sample_time = now;

    // Read sensor data and convert to original voltage (before voltage divider)
    // ADS1015 GAIN_TWOTHIRDS: 3mV per LSB, voltage divider doubles the original voltage
    float v0_original = ads.readADC_SingleEnded(0) * 0.003f * 2.0f; // Convert to original 0-5V
    float v1_original = ads.readADC_SingleEnded(1) * 0.003f * 2.0f;
    
    // Convert voltage to pressure: 1V=0MPa, 5V=1MPa -> P = (V-1)/4
    float p0 = (v0_original - 1.0f) / 4.0f; // Pressure in MPa
    float p1 = (v1_original - 1.0f) / 4.0f;
    
    // Constrain pressure to 0-1.0 MPa range
    p0 = constrain(p0, 0.0f, 1.0f);
    p1 = constrain(p1, 0.0f, 1.0f);
    
    // Process data through state machine
    stateManager.processSensorData(p0, p1, now);
  }
}