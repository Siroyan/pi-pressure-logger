#include <M5Stack.h>
#include <Adafruit_ADS1X15.h>

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
    M5.Lcd.fillRect(30, 210, 280, 20, BLACK);
    
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(30, 210);
    M5.Lcd.printf("CH0: %.2fV, CH1: %.2fV", v0, v1);
    M5.Lcd.setTextSize(1);
    
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

  drawLabels();
}

void loop() {
  unsigned long now = millis();
  if (now - last_sample_time >= interval_ms) {
    last_sample_time = now;

    float v0 = ads.readADC_SingleEnded(0) * 0.002f * voltage_scale;
    float v1 = ads.readADC_SingleEnded(1) * 0.002f * voltage_scale;

    ch0_buffer[buf_index] = v0;
    ch1_buffer[buf_index] = v1;

    drawOnePoint(buf_index, v0, v1);
    drawVoltageText(v0, v1);

    buf_index = (buf_index + 1) % buffer_size;
  }
}
