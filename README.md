# pi-pressure-logger

M5Stack Core ESP32 と ADS1015 を使って、2 チャンネルの圧力を測定・表示・記録する PlatformIO プロジェクトです。

- CH0 / CH1 の圧力を 目標100 Hz（10 ms周期）で読み取る
- M5Stack の LCD に波形と現在値を表示する
- SD カードへ CSV 形式で保存する
- Wi-Fi 経由で AWS IoT Core へ MQTT 送信する
- NTP で時刻を取得し、ログファイル名に利用する
- M5Stack のボタンから記録状態と SD カード内のファイルを操作する

## 動作環境

- M5Stack Core ESP32
- ADS1015
- 1〜5 V 出力の圧力センサー 2 台
- 47 kΩ / 47 kΩ の分圧回路（センサー出力を ADS1015 の入力範囲に合わせる）
- microSD カード
- PlatformIO（VS Code 拡張または CLI）
- AWS IoT Core を使う場合は Wi-Fi と AWS IoT の証明書

圧力センサーの入力は、コード上では「1 V = 0 MPa、5 V = 1 MPa」として変換します。波形だけを 0〜0.5 MPa に切り詰め、数値表示・CSV保存・MQTT送信には変換後の値をそのまま使います。センサーの仕様が異なる場合は圧力変換式を変更してください。

ADS1015 の入力には分圧後の電圧を接続します。分圧比やセンサーの出力範囲が異なる場合、実際の入力電圧が ADS1015 の許容範囲を超えないことを確認してください。

### I2C配線と電源

| M5Stack Core | ADS1015 |
| --- | --- |
| GPIO21（SDA） | SDA |
| GPIO22（SCL） | SCL |
| GND | GND、ADDR（アドレス0x48） |
| 3.3 V | VDD（3.3 V動作に対応した基板を使用） |

CH0／CH1はそれぞれAIN0／AIN1へ分圧後の信号を接続し、センサー側GNDも共通にします。I2CのプルアップはESP32に合わせて3.3 Vとし、M5StackのPort Aにある5 V端子を3.3 V端子と取り違えないでください。ADS1015の±6.144 Vというゲイン設定は、電源電圧を超える入力を許可するものではありません。回路は[M5Stack Basic公式仕様](https://docs.m5stack.com/en/core/basic)と[TI ADS1015データシート](https://www.ti.com/lit/ds/symlink/ads1015.pdf)に照らして確認してください。

## セットアップ

### 1. リポジトリを取得

ADS1015 のライブラリを Git サブモジュールとして使用しています。

```bash
git clone <repository-url>
cd pi-pressure-logger
git submodule update --init --recursive
```

### 2. 秘密情報ファイルを作成

`secure/` 以下の実ファイルは `.gitignore` で除外されています。テンプレートをコピーして、実際の値を設定してください。

```bash
cp secure/config.h.example secure/config.h
cp secure/aws_certificates.h.example secure/aws_certificates.h
```

`secure/config.h` に Wi-Fi と AWS IoT の接続情報を設定します。

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* aws_iot_endpoint = "YOUR_AWS_IOT_ENDPOINT.iot.region.amazonaws.com";
const int aws_iot_port = 8883;
const char* thing_name = "PressureLogger";
const char* aws_iot_topic = "pressure_logger/data";
```

`secure/aws_certificates.h` には、AWS IoT Core の Amazon Root CA、デバイス証明書、秘密鍵を設定します。証明書の発行・ポリシー設定・アタッチ方法は [AWS_SETUP.md](AWS_SETUP.md) を参照してください。

証明書と秘密鍵はリポジトリへコミットしないでください。`secure/` は認証情報を置くためのローカル専用ディレクトリです。

### 3. ビルドと書き込み

```bash
pio run
pio run --target upload
pio device monitor
```

シリアルモニターの通信速度は `platformio.ini` の設定により 115200 baud です。

### ローカル検証

`bash scripts/check.sh`でホスト回帰テストと公開ダミー設定によるESP32ビルドを行います。秘密情報は不要です。手順と検証範囲は[ローカル検証](docs/LOCAL_TESTS.md)を参照してください。

## 使い方

起動すると、Wi-Fi、NTP、AWS IoT、SD カードを初期化し、LCD に CH0 / CH1 の波形を表示します。ネットワーク接続は切断時に自動再接続します。AWS IoT への MQTT 接続も 5 秒間隔で再試行します。

### 通常画面

- **A**: 待機中と記録中を切り替える
- **B**: SD カードのファイル一覧を開く
- **C**: 通常画面では使用しない

記録中は SD カードへの保存と MQTT 送信を行います。B ボタンは記録中の誤操作を防ぐため、状態を変更しません。

### ファイル一覧

- **A**: 次のファイルを選択（末尾から先頭へ循環）
- **B**: 選択中の CSV ファイルを削除
- **C**: 通常画面へ戻る

ファイル一覧には新しいファイルから表示され、1 ページに最大 8 ファイルを表示します。削除操作に確認画面はありません。

## 計測と保存

センサー値は次の条件で処理されます。

- サンプリング目標: 100 Hz（10 ms周期。RTOS・I/O負荷による遅延や欠測を除く）
- 波形バッファ: 20 秒分（2,000 サンプル）
- 波形の最新位置を示す空白: 最古側の1秒分
- 圧力単位: MPa
- 波形表示範囲: 0〜0.5 MPa
- 保存・送信値: 変換後の値を制限せず記録・送信

SD カードには `pressure_log_YYYY-MM-DD-HH-MM-SS.csv` という名前で保存します。NTP 同期に失敗した場合は、起動後の `millis()` を使ったファイル名にフォールバックします。

CSV の形式は次のとおりです。

```csv
Timestamp(ms),CH0(MPa),CH1(MPa)
0,0.1234,0.2345
10,0.6235,-0.0100
```

`Timestamp(ms)` は記録開始からの経過時間です。NTP 同期済みの場合、ファイルの先頭には記録開始時刻を示すコメント行も追加されます。

## AWS IoT MQTT

記録中、接続済みであれば 500 ms 間隔で JSON ペイロードを `pressure_logger/data` トピックへ送信します。送信先は `secure/config.h` の `aws_iot_topic` で設定できます。

```json
{
  "timestamp": 123456789,
  "device": "PressureLogger",
  "ch0": 0.1234,
  "ch1": 0.2345
}
```

`timestamp` は Unix 時刻ではなく、デバイス起動後の `millis()` です。AWS IoT ポリシーには、Thing の接続権限と設定したトピックへの Publish 権限を付与してください。ポリシーの例は [AWS_SETUP.md](AWS_SETUP.md) にあります。

## プロジェクト構成

```text
src/
├── main.cpp              # 初期化、センサー読み取り、ボタン処理
├── StateManager.*        # STANDBY / RECORDING / FILE_LIST の状態管理
├── DisplayManager.*      # 波形、状態、ファイル一覧の表示
├── SDManager.*           # CSV 作成、記録、一覧、削除
├── WiFiManager.*         # Wi-Fi 接続と再接続
├── MQTTManager.*         # AWS IoT MQTT 接続と送信
└── TimeManager.*         # NTP 同期と時刻整形
secure/
├── config.h.example      # Wi-Fi / AWS IoT 設定のテンプレート
└── aws_certificates.h.example # 証明書のテンプレート
lib/Adafruit_ADS1X15/     # ADS1015 ライブラリ（Git サブモジュール）
```

## トラブルシューティング

- `config.h` や `aws_certificates.h` が見つからない場合は、テンプレートから実ファイルを作成してください。
- ADS1015 のヘッダが見つからない場合は、Git サブモジュールを初期化してください。
- SD が `SD:ERR` になる場合は、カードの挿入状態、フォーマット、接触を確認してください。
- Wi-Fi や AWS が接続できない場合は、認証情報、AWS IoT エンドポイント、証明書、Thing 名、IoT ポリシーを確認してください。
- TLS 接続に失敗する場合は、Root CA・デバイス証明書・秘密鍵の BEGIN / END マーカーを含めて正しく貼り付けてください。

## ライセンスと依存ライブラリ

依存ライブラリは `platformio.ini` に定義されています。

- M5Stack
- Adafruit BusIO
- Adafruit ADS1X15（Git サブモジュール）
- PubSubClient

各ライブラリのライセンスと利用条件にも従ってください。
