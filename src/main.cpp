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
#include "StorageService.h"
#include "RuntimeStartup.h"
#include <freertos/queue.h>
#include <freertos/task.h>

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


const int sample_queue_size = 512;
const int max_samples_per_loop = 4;
RecordingQueue recordingQueue;


// Button debounce variables
unsigned long button_press_time = 0;
const int min_press_duration = 50; // Minimum press duration in ms to be considered valid

// Manager instances
WiFiClientSecure wifiClientSecure;
WiFiManager wifiManager(ssid, password);
MQTTManager mqttManager(&wifiClientSecure, aws_iot_endpoint, aws_iot_port, thing_name,
                        aws_iot_topic, aws_root_ca, device_cert, device_key);
SDManager sdManager;
StorageService storageService(sdManager, recordingQueue);
std::vector<String> listedFiles;
std::vector<long> listedSizes;
bool fileRequestPending = false;
bool screenDirty = true;
bool fileOperationSuccess = true;
DisplayManager displayManager;
TimeManager timeManager;
StateManager stateManager;
RuntimeStartup runtime;
FileAction fileAction;

void requestFileList() {
  fileRequestPending = runtime.storage && storageService.requestList();
  fileOperationSuccess = fileRequestPending;
  screenDirty = true;
}

void handleButtonInput() {
  if (stateManager.getCurrentState() == FILE_LIST && fileAction.status() != FileAction::Status::None) {
    if (M5.BtnC.wasPressed() && fileAction.dismiss()) requestFileList();
    else if (M5.BtnB.wasPressed() && fileAction.confirm()) {
      fileRequestPending = runtime.storage && storageService.requestDelete(fileAction.filename());
      if (!fileRequestPending) fileAction.complete(false);
    }
    screenDirty = true;
    button_press_time = 0;
    return;
  }
  if (M5.BtnA.wasPressed()) button_press_time = millis();
  if (M5.BtnA.wasReleased() && button_press_time > 0) {
    if (millis() - button_press_time >= min_press_duration) {
      if (stateManager.getCurrentState() == FILE_LIST) {
        if (!fileRequestPending) {
          displayManager.navigateFileList(1, listedFiles.size());
          screenDirty = true;
        }
      } else if (adc_available || stateManager.getCurrentState() == RECORDING) {
        stateManager.toggleState();
        screenDirty = true;
      }
    }
    button_press_time = 0;
  }
  if (M5.BtnB.wasPressed()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      int selected = displayManager.getSelectedFileIndex();
      if (!fileRequestPending && selected >= 0 && selected < (int)listedFiles.size()) {
        fileAction.begin(listedFiles[selected],listedSizes[selected]);
        screenDirty = true;
      }
    } else if (stateManager.getCurrentState() == STANDBY) {
      stateManager.transitionToFileList();
      displayManager.resetFileListNavigation();
      requestFileList();
    }
  }
  if (M5.BtnC.wasPressed() && stateManager.getCurrentState() == FILE_LIST) {
    stateManager.transitionToStandby();
    screenDirty = true;
  }
}

void storageTask(void*) {
  while (true) {
    storageService.step();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void handleNetworkMaintenance() {
  // Check WiFi connection and maintain MQTT
  wifiManager.checkConnection();
  timeManager.poll();
  RecordEvent event;
  if (recordingQueue.receiveNetwork(event)) {
    if (event.kind == RecordKind::Sample && event.session) {
      mqttManager.offer({event.sample.p0,event.sample.p1,event.sample.timestamp,event.session,event.sequence});
    } else mqttManager.clearPending();
  }
  mqttManager.loop(wifiManager.isConnected() && timeManager.isTimeSynced());
}

void samplingTask(void* parameter) {
  TickType_t last_wake_time = xTaskGetTickCount();
  const TickType_t sample_period = pdMS_TO_TICKS(interval_ms);

  while (true) {
    vTaskDelayUntil(&last_wake_time, sample_period);

    float v0_original = ads.readADC_SingleEnded(0) * 0.003f * 2.0f;
    float v1_original = ads.readADC_SingleEnded(1) * 0.003f * 2.0f;

    recordingQueue.submit((v0_original - 1.0f) / 4.0f,
                          (v1_original - 1.0f) / 4.0f);
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

  stateManager.setRecordingQueue(&recordingQueue);
  runtime = startStorageRuntime(adc_available,mqttManager.isReady(), [] {
    return recordingQueue.init(sample_queue_size);
  }, [] {
    return storageService.init();
  }, [] {
    return xTaskCreatePinnedToCore(storageTask,"pressure-storage",6144,nullptr,1,nullptr,1)==pdPASS;
  }, [] {
    return xTaskCreatePinnedToCore(samplingTask,"pressure-sampling",4096,nullptr,3,nullptr,1)==pdPASS;
  }, [] {
    return xTaskCreatePinnedToCore(networkTask,"network-maintenance",8192,nullptr,tskIDLE_PRIORITY,nullptr,0)==pdPASS;
  });
  stateManager.setRecordingReady(runtime.acquisition);
  if (runtime.error) Serial.println(runtime.error);

}

void loop() {
  M5.update();
  unsigned long now = millis();
  
  // Handle user input
  handleButtonInput();
  
  if (storageService.takeResult(listedFiles, listedSizes, fileOperationSuccess)) {
    fileRequestPending = false;
    if (fileAction.status()==FileAction::Status::Busy) fileAction.complete(fileOperationSuccess);
    displayManager.resetFileListNavigation();
    screenDirty = true;
  }
  // SD and LCD share SPI on the M5Stack. Skip drawing rather than blocking
  // button polling on the SD driver's bus transaction.
  if (!runtime.storage || storageService.tryBeginDisplay()) {
    if (stateManager.getCurrentState() == FILE_LIST) {
      if (screenDirty) {
        if (fileAction.status()!=FileAction::Status::None) {
          displayManager.drawFileAction(fileAction);
        } else if (fileRequestPending) {
          M5.Lcd.fillScreen(BLACK); M5.Lcd.setCursor(10,10); M5.Lcd.print("Loading... C:Back");
        } else {
          displayManager.drawFileList(listedFiles, listedSizes);
          if (!fileOperationSuccess) { M5.Lcd.setCursor(10,200); M5.Lcd.print("Storage operation failed"); }
        }
        screenDirty = false;
      }
    } else {
      if (screenDirty) { displayManager.init(); screenDirty = false; }
      displayManager.advanceGraph(acquisitionMillis());
      static unsigned long last_status_update = 0;
      if (now - last_status_update >= 500) {
        last_status_update = now;
        M5.Lcd.fillRect(30,200,280,9,BLACK);
        if (runtime.error) { M5.Lcd.setCursor(30,200); M5.Lcd.print(runtime.error); }
        displayManager.drawConnectionStatus(adc_available, sdManager.isAvailable(), sdManager.isRecording(),
                                            sdManager.hasWriteError(),
                                            wifiManager.isConnected(), mqttManager.isConnected());
      }
    }
    PressureSample sample;
    for (int i=0; i<max_samples_per_loop && recordingQueue.receiveDisplay(sample); ++i) {
      stateManager.processSensorData(sample.p0, sample.p1, sample.timestamp);
    }
    if (runtime.storage) storageService.endDisplay();
  }
  static unsigned long last_drop_report = 0;
  if (now - last_drop_report >= 5000) {
    last_drop_report = now;
    Serial.printf("Session %u: storage queue high-water %u, total dropped %u\n",
                  recordingQueue.sessionId(), recordingQueue.maxDepth(), recordingQueue.droppedSamples());
  }

}
