#include <M5Stack.h>
#include <Adafruit_ADS1X15.h>
#ifdef PRESSURE_TEST_CONFIG
#include "../test/support/FirmwareConfig.h"
#else
#include "../secure/aws_certificates.h"
#include "../secure/config.h"
#endif
#include "StateManager.h"
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "SDManager.h"
#include "DisplayManager.h"
#include "TimeManager.h"
#include "RuntimeStartup.h"
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_timer.h>

Adafruit_ADS1015 ads;
bool adc_available = false;
// Voltage divider: 5V -> 2.5V (R1=47k, R2=47k)
// ADS1015 with GAIN_TWOTHIRDS: 0-6.144V range, LSB = 3mV
// Pressure conversion: 1V=0MPa, 5V=1MPa

const int duration_sec = 20;
const int sampling_rate = 100;
const int buffer_size = duration_sec * sampling_rate;
const int graph_gap_samples = sampling_rate;

float ch0_buffer[buffer_size] = {0};
float ch1_buffer[buffer_size] = {0};
int buf_index = 0;

const int interval_ms = 1000 / sampling_rate;

struct PressureSample {
  float p0;
  float p1;
  uint64_t timestamp;
};

const int sample_queue_size = 512;
const int max_samples_per_loop = 4;
QueueHandle_t sample_queue = nullptr;
volatile unsigned long dropped_sample_count = 0;

// Button debounce variables
unsigned long button_press_time = 0;
const int min_press_duration = 50; // Minimum press duration in ms to be considered valid

// Manager instances
WiFiClientSecure wifiClientSecure;
WiFiManager wifiManager(ssid, password);
MQTTManager mqttManager(&wifiClientSecure, aws_iot_endpoint, aws_iot_port, thing_name,
                        aws_iot_topic, aws_root_ca, device_cert, device_key);
SDManager sdManager;
DisplayManager displayManager;
TimeManager timeManager;
StateManager stateManager;
FileAction fileAction;
RuntimeStartup runtime;

void handleButtonInput() {
  if (stateManager.getCurrentState() == FILE_LIST && fileAction.status() != FileAction::Status::None) {
    if (M5.BtnC.wasPressed()) {
      if (fileAction.dismiss()) {
        auto files=sdManager.getLogFileList(); std::vector<long> sizes;
        for (const auto& name:files) sizes.push_back(sdManager.getFileSize(name));
        displayManager.resetFileListNavigation();
        displayManager.drawFileList(files,sizes);
      }
    } else if (M5.BtnB.wasPressed() && fileAction.confirm()) {
      displayManager.drawFileAction(fileAction);
      fileAction.complete(sdManager.deleteFile(fileAction.filename()));
      displayManager.drawFileAction(fileAction);
    }
    button_press_time=0;
    return;
  }
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
        if (adc_available) {
          stateManager.toggleState();
        } else {
          Serial.println("Recording unavailable: ADS1015 initialization failed");
        }
        displayManager.drawConnectionStatus(adc_available, sdManager.isAvailable(), sdManager.isRecording(),
                                            sdManager.hasWriteError(),
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
          
          fileAction.begin(selectedFile,sdManager.getFileSize(selectedFile));
          displayManager.drawFileAction(fileAction);
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
        displayManager.drawConnectionStatus(adc_available, sdManager.isAvailable(), sdManager.isRecording(),
                                            sdManager.hasWriteError(),
                                            wifiManager.isConnected(), mqttManager.isConnected());
      }
    }
  }
  
  // Handle Button C for returning to waveform screen
  if (M5.BtnC.wasPressed()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      // Return to STANDBY (waveform screen)
      stateManager.transitionToStandby();
      displayManager.drawConnectionStatus(adc_available, sdManager.isAvailable(), sdManager.isRecording(),
                                          sdManager.hasWriteError(),
                                          wifiManager.isConnected(), mqttManager.isConnected());
    }
  }
}

void handleNetworkMaintenance() {
  // Check WiFi connection and maintain MQTT
  wifiManager.checkConnection();
  timeManager.poll();
  mqttManager.loop(wifiManager.isConnected() && timeManager.isTimeSynced());
}

void samplingTask(void* parameter) {
  TickType_t last_wake_time = xTaskGetTickCount();
  const TickType_t sample_period = pdMS_TO_TICKS(interval_ms);

  while (true) {
    vTaskDelayUntil(&last_wake_time, sample_period);

    float v0_original = ads.readADC_SingleEnded(0) * 0.003f * 2.0f;
    float v1_original = ads.readADC_SingleEnded(1) * 0.003f * 2.0f;

    PressureSample sample;
    sample.p0 = (v0_original - 1.0f) / 4.0f;
    sample.p1 = (v1_original - 1.0f) / 4.0f;
    sample.timestamp = static_cast<uint64_t>(esp_timer_get_time()) / 1000;

    if (xQueueSend(sample_queue, &sample, 0) != pdTRUE) {
      dropped_sample_count++;
    }
  }
}

void networkTask(void* parameter) {
  while (true) {
    handleNetworkMaintenance();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup() {
  M5.begin();
  
  // Initialize sensor
  adc_available = ads.begin();
  if (adc_available) {
    ads.setDataRate(RATE_ADS1015_3300SPS);
    ads.setGain(GAIN_TWOTHIRDS); // 0-6.144V range for 0-2.5V input
  } else {
    Serial.println("ADS1015 initialization failed; sensor sampling disabled");
  }

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
  displayManager.drawConnectionStatus(adc_available, sdManager.isAvailable(), sdManager.isRecording(),
                                      sdManager.hasWriteError(),
                                      wifiManager.isConnected(), mqttManager.isConnected());

  runtime = startRuntime(adc_available, mqttManager.isReady(), [] {
    sample_queue = xQueueCreate(sample_queue_size, sizeof(PressureSample));
    return sample_queue != nullptr;
  }, [] {
    return xTaskCreatePinnedToCore(samplingTask, "pressure-sampling", 4096, nullptr, 3, nullptr, 1) == pdPASS;
  }, [] {
    // Idle priority keeps the Core 0 watchdog serviced during TLS connection.
    return xTaskCreatePinnedToCore(networkTask, "network-maintenance", 8192, nullptr,
                                   tskIDLE_PRIORITY, nullptr, 0) == pdPASS;
  });
  stateManager.setRecordingReady(runtime.acquisition);
  if (runtime.error) Serial.println(runtime.error);

}

void loop() {
  M5.update();
  unsigned long now = millis();
  
  // Handle user input
  handleButtonInput();
  if (stateManager.getCurrentState() != FILE_LIST)
    displayManager.advanceGraph(static_cast<uint64_t>(esp_timer_get_time()) / 1000);
  
  // Update connection status display periodically
  static unsigned long last_status_update = 0;
  if (stateManager.getCurrentState() != FILE_LIST && now - last_status_update >= 500) {
    last_status_update = now;
    M5.Lcd.fillRect(30,200,280,9,BLACK);
    if (runtime.error) { M5.Lcd.setCursor(30,200); M5.Lcd.print(runtime.error); }
    displayManager.drawConnectionStatus(adc_available, sdManager.isAvailable(), sdManager.isRecording(),
                                        sdManager.hasWriteError(),
                                        wifiManager.isConnected(), mqttManager.isConnected());
  }
  
  PressureSample sample;
  int processed_samples = 0;
  while (sample_queue && processed_samples < max_samples_per_loop &&
         xQueueReceive(sample_queue, &sample, 0) == pdTRUE) {
    stateManager.processSensorData(sample.p0, sample.p1, sample.timestamp);
    processed_samples++;
  }
  static unsigned long last_drop_report = 0;
  static unsigned long reported_drop_count = 0;
  if (now - last_drop_report >= 5000 && dropped_sample_count != reported_drop_count) {
    last_drop_report = now;
    reported_drop_count = dropped_sample_count;
    Serial.println("Dropped pressure samples: " + String(reported_drop_count));
  }
}
