// M5StickC Plus2 + ADC Hat (ADS1110) Voltage Logger @240Hz
// - Input: 1–5 V (ADC Hat 0–12 Vレンジを使用。簡易係数でスケール)
// - Sample rate: 240 SPS (ADS1110 continuous mode)
// - Display: 波形を横240px（=約1秒分）、縦135pxに描画（横向き）

#include <M5StickCPlus2.h>
#include <Wire.h>

// ====== ADS1110 (ADC Hat V1.1) ======
static const uint8_t ADS1110_ADDR = 0x48;  // ADC Hat デフォルト
// コンフィグレジスタ（連続変換, 240SPS, PGA=1）
// bit7 ST/DRDY(任意=1), bit6-5=0, bit4 SC=0(continuous), bit3-2 DR=00(240SPS), bit1-0 PGA=00(×1)
static const uint8_t ADS1110_CFG_240SPS = 0x80; // 1000 0000b

// 追加（回路定数）
static const float R_TOP = 510000.0f;   // 510k
static const float R_BOT = 100000.0f;   // 100k
static const float VREF  = 2.048f;

// 画面・波形設定
static const int WAVE_W = 240; // 横向きで幅240px（=約1秒分）
static const int WAVE_H = 100; // 上にグラフ領域100px確保
static const int WAVE_X = 0;
static const int WAVE_Y = 20;
static const float V_MIN = 1.0f;
static const float V_MAX = 5.0f;

// バッファ
float samples[WAVE_W];
int write_idx = 0;

// ==== I2C: StickC Plus2 HAT側（SDA=G32, SCL=G33） ====
static const int PIN_SDA = 0;
static const int PIN_SCL = 26;

// ==== 240Hz制御 ====
static const uint32_t SAMPLE_INTERVAL_US = 1000000UL / 240; // ≒4167us
uint32_t next_us = 0;

// ADS1110初期化
bool ads1110_begin() {
  Wire.beginTransmission(ADS1110_ADDR);
  Wire.write(ADS1110_CFG_240SPS);
  if (Wire.endTransmission() != 0) return false;
  // 変換が回り始めるのを少し待つ
  delay(10);
  return true;
}

// ADS1110から最新値を読む（出力2byte + cfg1byte）
// ST/DRDYが0=新データ、1=既読データ（datasheet記載）
bool ads1110_readRaw(int16_t &raw, uint8_t &cfg) {
  Wire.requestFrom((int)ADS1110_ADDR, 3);
  if (Wire.available() < 3) return false;
  uint8_t msb = Wire.read();
  uint8_t lsb = Wire.read();
  cfg = Wire.read();
  raw = (int16_t)((msb << 8) | lsb);
  return true;
}

// 置き換え
float convert_to_volts(int16_t code, uint8_t cfg) {
  // (1) データレートに応じたフルスケールカウント
  const uint8_t dr = (cfg >> 2) & 0x03; // 00:240, 01:60, 10:30, 11:15 SPS
  static const int FS_COUNTS[4] = {2048, 8192, 16384, 32768};
  // (2) PGA
  const float gain = 1 << (cfg & 0x03); // 00:×1,01:×2,10:×4,11:×8
  // (3) ADC入力電圧（右詰めコードを正しい分母で）
  const float v_adc = (float)code * (VREF / (gain * FS_COUNTS[dr]));
  // (4) 端子電圧へ（分圧補正）
  return v_adc * ((R_TOP + R_BOT) / R_BOT); // ≒ ×6.1
}

// 値を[1–5V]にクリップしてY座標へ
int v_to_y(float v) {
  float vc = v;
  if (vc < V_MIN) vc = V_MIN;
  if (vc > V_MAX) vc = V_MAX;
  // 上が5Vで下が1Vになるように反転マッピング
  float t = (vc - V_MIN) / (V_MAX - V_MIN); // 0..1
  int y = WAVE_Y + WAVE_H - 1 - (int)(t * (WAVE_H - 1));
  return y;
}

// 波形を1pxスクロール描画（軽量）
// - 背景スクロールは fillRect で横一列を消してから新ドット描画
void draw_wave_step(int newY) {
  // 左へ1pxスクロール
  M5.Display.scroll(-1, 0);
  // 右端の縦帯をクリア
  M5.Display.fillRect(WAVE_X + WAVE_W - 1, WAVE_Y, 1, WAVE_H, BLACK);
  // 新しい点を右端に描画
  M5.Display.drawPixel(WAVE_X + WAVE_W - 1, newY, GREEN);
}

// 目盛りと枠を再描画（起動時のみ）
void draw_frame() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(1);
  // タイトル
  M5.Display.setCursor(4, 2);
  M5.Display.print("Voltage Logger 240Hz (1-5V)");

  // 枠
  M5.Display.drawRect(WAVE_X, WAVE_Y, WAVE_W, WAVE_H, DARKGREY);
  // 1–5Vの水平グリッド（5本）
  for (int i = 0; i <= 4; ++i) {
    float v = V_MIN + i * (V_MAX - V_MIN) / 4.0f;
    int y = v_to_y(v);
    M5.Display.drawLine(WAVE_X, y, WAVE_X + WAVE_W - 1, y, 0x4208 /*薄い灰*/);
    M5.Display.setCursor(WAVE_X + 2, y - 6);
    M5.Display.printf("%.1fV", v);
  }
  // 右端の凡例エリア（下段に数値）
  M5.Display.fillRect(0, WAVE_Y + WAVE_H + 1, 240, 135 - (WAVE_Y + WAVE_H + 1), BLACK);
}

void setup() {
  Serial.begin(9600);

  auto cfg = M5.config();
  M5.begin(cfg);
  // 画面を横向き（長辺=240）に
  M5.Display.setRotation(1);

  Wire.begin(PIN_SDA, PIN_SCL, 400000); // 400kHz I2C

  if (!ads1110_begin()) {
    M5.Display.setRotation(0);
    M5.Display.fillScreen(BLACK);
    M5.Display.setTextColor(RED, BLACK);
    M5.Display.setTextSize(2);
    M5.Display.setCursor(8, 50);
    M5.Display.println("ADS1110 not found!");
    while (1) { delay(1000); }
  }

  draw_frame();

  // バッファ初期化
  for (int i = 0; i < WAVE_W; ++i) samples[i] = NAN;

  next_us = micros();
}

void loop() {
  // 240Hz間隔を維持
  const uint32_t now = micros();
  if ((int32_t)(now - next_us) < 0) return;
  next_us += SAMPLE_INTERVAL_US;

  // ADC読み
  int16_t code;
  uint8_t cfg;
  if (!ads1110_readRaw(code, cfg)) return;

  Serial.println(cfg);
  // ST/DRDY==0 なら新データ
  if ((cfg & 0x80) == 0) {
    float vin = convert_to_volts(code, cfg);

    // 表示用の数値（下段）
    M5.Display.fillRect(120, WAVE_Y + WAVE_H + 6, 116, 12, BLACK);
    M5.Display.setTextColor(YELLOW, BLACK);
    M5.Display.setCursor(120, WAVE_Y + WAVE_H + 6);
    M5.Display.printf("V=%.3f", vin);

    // 波形1ステップ描画
    int y = v_to_y(vin);
    draw_wave_step(y);

    // サンプル保持（必要なら保存/記録用に使う）
    samples[write_idx] = vin;
    write_idx = (write_idx + 1) % WAVE_W;
  }

  // 1–5Vの水平グリッド（5本）
  for (int i = 0; i <= 4; ++i) {
    float v = V_MIN + i * (V_MAX - V_MIN) / 4.0f;
    int y = v_to_y(v);
    M5.Display.drawLine(WAVE_X, y, WAVE_X + WAVE_W - 1, y, 0x4208 /*薄い灰*/);
    M5.Display.setCursor(WAVE_X + 2, y - 6);
    M5.Display.printf("%.1fV", v);
  }

  // ボタンAでフレーム再描画（画面が乱れた時のリフレッシュ）
  M5.update();
  if (M5.BtnA.wasPressed()) {
    draw_frame();
  }
}
