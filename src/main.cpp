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
const float voltage_scale = 5.7;

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
      stateManager.toggleState();
      displayManager.drawConnectionStatus(sdManager.isAvailable(), sdManager.isRecording(), 
                                          wifiManager.isConnected(), mqttManager.isConnected());
    } else {
      Serial.println("BtnA press too short (" + String(press_duration) + "ms) - ignored");
    }
    button_press_time = 0;
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
  ads.setGain(GAIN_ONE);

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

    // Read sensor data
    float v0 = ads.readADC_SingleEnded(0) * 0.002f * voltage_scale;
    float v1 = ads.readADC_SingleEnded(1) * 0.002f * voltage_scale;
    
    // Process data through state machine
    stateManager.processSensorData(v0, v1, now);
  }
}