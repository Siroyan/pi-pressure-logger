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

unsigned long last_sample_time = 0;
const int interval_ms = 1000 / sampling_rate;

void drawGraphFrame() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.drawRect(10, 20, 300, 80, WHITE);   // CH0
  M5.Lcd.drawRect(10, 120, 300, 80, WHITE);  // CH1

  M5.Lcd.setCursor(320, 20);  M5.Lcd.print("CH0");
  M5.Lcd.setCursor(320, 120); M5.Lcd.print("CH1");

  M5.Lcd.setCursor(5, 10);    M5.Lcd.printf("12V");
  M5.Lcd.setCursor(5, 90);    M5.Lcd.printf("0V");
  M5.Lcd.setCursor(5, 110);   M5.Lcd.printf("12V");
  M5.Lcd.setCursor(5, 180);   M5.Lcd.printf("0V");

  M5.Lcd.setCursor(10, 210);
  M5.Lcd.printf("CH0:      V    CH1:      V");
}

void drawOnePoint(int i, float v0, float v1) {
  // 0V〜12Vに制限
  v0 = constrain(v0, 0.0, 12.0);
  v1 = constrain(v1, 0.0, 12.0);

  int x = 10 + (i * 300 / buffer_size);

  // 過去の波形をしっかり消去（上下端も含めて）
  M5.Lcd.fillRect(x, 19, 1, 82, BLACK);   // CH0
  M5.Lcd.fillRect(x, 119, 1, 82, BLACK);  // CH1

  // ピクセル描画（12V = 上, 0V = 下）
  int y0 = 20 + 80 - (v0 / 12.0f) * 80;
  int y1 = 120 + 80 - (v1 / 12.0f) * 80;

  M5.Lcd.drawPixel(x, y0, GREEN);
  M5.Lcd.drawPixel(x, y1, CYAN);
}

void drawVoltageText(float v0, float v1) {
  M5.Lcd.fillRect(45, 210, 60, 10, BLACK);
  M5.Lcd.fillRect(175, 210, 60, 10, BLACK);

  M5.Lcd.setCursor(45, 210);
  M5.Lcd.printf("%.2f", v0);

  M5.Lcd.setCursor(175, 210);
  M5.Lcd.printf("%.2f", v1);
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

  drawGraphFrame();
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
