#include <M5Stack.h>
#include <Adafruit_ADS1X15.h>
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "../secure/aws_certificates.h"

Adafruit_ADS1015 ads;
const float voltage_scale = 5.7;

const int duration_sec = 20;
const int sampling_rate = 100;
const int buffer_size = duration_sec * sampling_rate;

float ch0_buffer[buffer_size] = {0};
float ch1_buffer[buffer_size] = {0};
int buf_index = 0;

float last_displayed_v0 = -1.0;
float last_displayed_v1 = -1.0;

bool sd_available = false;
String log_filename = "";
unsigned long session_start_time = 0;
bool recording = false;

// WiFi and AWS IoT Core configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aws_iot_endpoint = "YOUR_AWS_IOT_ENDPOINT.iot.region.amazonaws.com";
const int aws_iot_port = 8883;
const char* aws_iot_topic = "pressure_logger/data";
const char* thing_name = "PressureLogger";

bool wifi_connected = false;
bool mqtt_connected = false;
WiFiClientSecure wifiClientSecure;
PubSubClient client(wifiClientSecure);
unsigned long last_mqtt_attempt = 0;
const int mqtt_retry_interval = 5000;


unsigned long last_sample_time = 0;
const int interval_ms = 1000 / sampling_rate;

void drawLabels() {
  M5.Lcd.setCursor(30, 10);   M5.Lcd.print("CH0");
  M5.Lcd.setCursor(30, 110);  M5.Lcd.print("CH1");


  M5.Lcd.drawRect(30, 20, 280, 80, WHITE);   // CH0
  M5.Lcd.drawRect(30, 120, 280, 80, WHITE);  // CH1
  
  // Scale marks and legends on left border
  M5.Lcd.setTextColor(0x7BEF);  // Gray color
  
  // CH0 scale marks (0V, 3V, 6V, 9V, 12V)
  for (int i = 0; i <= 4; i++) {
    int y = 99 - (i * 78 / 4);  // y=99,79,59,39,20
    int voltage = i * 3;  // 0, 3, 6, 9, 12
    M5.Lcd.drawLine(28, y, 30, y, WHITE);  // Tick mark
    M5.Lcd.setCursor(8, y - 3);
    M5.Lcd.printf("%dV", voltage);
  }
  
  // CH1 scale marks (0V, 3V, 6V, 9V, 12V)
  for (int i = 0; i <= 4; i++) {
    int y = 199 - (i * 78 / 4);  // y=199,179,159,139,120
    int voltage = i * 3;  // 0, 3, 6, 9, 12
    M5.Lcd.drawLine(28, y, 30, y, WHITE);  // Tick mark
    M5.Lcd.setCursor(8, y - 3);
    M5.Lcd.printf("%dV", voltage);
  }
  
  M5.Lcd.setTextColor(WHITE);  // Reset to white
}

bool initializeSD() {
  if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }
  
  Serial.println("SD Card initialized successfully");
  return true;
}

String createLogFile() {
  // Create filename with timestamp
  unsigned long timestamp = millis();
  String filename = "/pressure_log_" + String(timestamp) + ".csv";
  
  File file = SD.open(filename.c_str(), FILE_WRITE);
  if (file) {
    // Write CSV header
    file.println("Timestamp(ms),CH0(V),CH1(V)");
    file.close();
    Serial.println("Created log file: " + filename);
    return filename;
  }
  
  Serial.println("Failed to create log file");
  return "";
}

void logData(float v0, float v1) {
  if (!sd_available || log_filename == "" || !recording) return;
  
  File file = SD.open(log_filename.c_str(), FILE_APPEND);
  if (file) {
    unsigned long timestamp = millis() - session_start_time;
    file.println(String(timestamp) + "," + String(v0, 3) + "," + String(v1, 3));
    file.close();
  }
}

void startRecording() {
  if (!sd_available) return;
  
  log_filename = createLogFile();
  if (log_filename != "") {
    recording = true;
    session_start_time = millis();
    Serial.println("Recording started");
  }
}

void stopRecording() {
  if (recording) {
    recording = false;
    Serial.println("Recording stopped");
  }
}

void initWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  unsigned long start_time = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_time < 10000) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifi_connected = true;
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    wifi_connected = false;
    Serial.println();
    Serial.println("WiFi connection failed!");
  }
}

void setupAWSIoT() {
  // Set certificates from header file
  wifiClientSecure.setCACert(aws_root_ca);
  wifiClientSecure.setCertificate(device_cert);
  wifiClientSecure.setPrivateKey(device_key);
  
  Serial.println("AWS IoT certificates loaded");
}

void reconnectMQTT() {
  if (!wifi_connected) return;
  
  if (millis() - last_mqtt_attempt < mqtt_retry_interval) return;
  last_mqtt_attempt = millis();
  
  Serial.print("Attempting AWS IoT connection...");
  
  if (client.connect(thing_name)) {
    mqtt_connected = true;
    Serial.println("connected to AWS IoT Core");
  } else {
    mqtt_connected = false;
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
  }
}

void publishMQTTData(float v0, float v1) {
  if (!mqtt_connected) return;
  
  String payload = "{";
  payload += "\"timestamp\":" + String(millis());
  payload += ",\"device\":\"" + String(thing_name) + "\"";
  payload += ",\"ch0\":" + String(v0, 3);
  payload += ",\"ch1\":" + String(v1, 3);
  payload += "}";
  
  if (client.publish(aws_iot_topic, payload.c_str())) {
    Serial.println("Data published to AWS IoT: " + payload);
  } else {
    Serial.println("Failed to publish data");
  }
}

void drawConnectionStatus() {
  M5.Lcd.fillRect(30, 225, 150, 10, BLACK);  // Clear area
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(30, 225);
  
  // SD Status
  if (sd_available) {
    if (recording) {
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

void drawOnePoint(int i, float v0, float v1) {
  // 0V〜12Vに制限
  v0 = constrain(v0, 0.0, 12.0);
  v1 = constrain(v1, 0.0, 12.0);

  int x = 31 + (i * 278 / buffer_size);  // x=31-308 (inside border)

  // 過去の波形をしっかり消去（内側のみ）
  M5.Lcd.fillRect(x, 21, 1, 78, BLACK);   // CH0 (y=21-98)
  M5.Lcd.fillRect(x, 121, 1, 78, BLACK);  // CH1 (y=121-198)

  // ピクセル描画（12V = 上, 0V = 下、内側領域に制限）
  int y0 = 21 + 78 - (v0 / 12.0f) * 78;  // y=21-98 (inside border)
  int y1 = 121 + 78 - (v1 / 12.0f) * 78; // y=121-198 (inside border)

  M5.Lcd.drawPixel(x, y0, GREEN);
  M5.Lcd.drawPixel(x, y1, CYAN);
}

void drawVoltageText(float v0, float v1) {
  if (abs(v0 - last_displayed_v0) > 0.01 || abs(v1 - last_displayed_v1) > 0.01) {
    M5.Lcd.fillRect(30, 210, 250, 15, BLACK);
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.setCursor(30, 210);
    M5.Lcd.printf("CH0: %.2fV, CH1: %.2fV", v0, v1);
    
    last_displayed_v0 = v0;
    last_displayed_v1 = v1;
  }
}

void setup() {
  M5.begin();
  M5.Lcd.setRotation(1);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(1);

  ads.begin();
  ads.setDataRate(RATE_ADS1015_3300SPS);
  ads.setGain(GAIN_ONE);

  // Initialize SD card
  sd_available = initializeSD();
  
  // Initialize WiFi
  initWiFi();
  
  // Setup AWS IoT certificates and connection
  setupAWSIoT();
  client.setServer(aws_iot_endpoint, aws_iot_port);

  drawLabels();
  drawConnectionStatus();
}

void loop() {
  M5.update();
  
  // Handle Button A for recording toggle
  if (M5.BtnA.wasPressed()) {
    if (recording) {
      stopRecording();
    } else {
      startRecording();
    }
    drawConnectionStatus();
  }
  
  // Maintain MQTT connection
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();
  
  unsigned long now = millis();
  if (now - last_sample_time >= interval_ms) {
    last_sample_time = now;

    float v0 = ads.readADC_SingleEnded(0) * 0.002f * voltage_scale;
    float v1 = ads.readADC_SingleEnded(1) * 0.002f * voltage_scale;

    ch0_buffer[buf_index] = v0;
    ch1_buffer[buf_index] = v1;

    drawOnePoint(buf_index, v0, v1);
    drawVoltageText(v0, v1);
    
    // Log data to SD card
    logData(v0, v1);
    
    // Publish data via MQTT
    publishMQTTData(v0, v1);

    buf_index = (buf_index + 1) % buffer_size;
  }
}
