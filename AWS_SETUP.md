# AWS IoT Core セットアップガイド

> 最終確認日: 2026-09-24
>
> AWS Management Consoleのメニュー名・画面構成・ボタン名は変更されることがあります。本書は上記日付時点のAWS公式ドキュメントを基準にしています。画面が一致しない場合は、本文中の公式リンクにある最新手順を優先してください。

この文書では、M5StackからAWS IoT CoreへMQTT over TLSで圧力データを送信し、AWS IoTコンソールで受信を確認するまでを説明します。

## このプロジェクトの接続仕様

| 項目 | 値 |
| --- | --- |
| Thing名 | `PressureLogger` |
| MQTTクライアントID | `PressureLogger`（`thing_name`の値） |
| 認証 | X.509デバイス証明書 |
| プロトコル／ポート | MQTT over TLS／`8883` |
| Publish先 | `pressure_logger/data`（`aws_iot_topic`の値） |
| 送信間隔 | 記録中に約500 ms間隔 |
| Subscribe | デバイス側では使用しない |

AWS IoT Coreは、X.509クライアント証明書を使うMQTT接続でポート8883をサポートしています。詳細は[AWS IoT Coreの通信プロトコル](https://docs.aws.amazon.com/iot/latest/developerguide/protocols.html)を参照してください。

## 前提条件

- AWSアカウント
- AWS IoT CoreのThing、証明書、ポリシーを作成できる権限
- M5Stackが接続できるWi-Fi
- PlatformIO
- AWS CLI（任意）

AWS IoT Coreのリソースはリージョンごとに管理されます。Thing、証明書、ポリシー、MQTTテストクライアントでは同じリージョンを選択してください。

## 1. リージョンとアカウントIDを確認する

ポリシー例にある値を自分の環境へ置き換えます。

- `YOUR_REGION`: 例 `ap-northeast-1`
- `YOUR_ACCOUNT_ID`: 12桁のAWSアカウントID

AWS CLIを設定済みの場合は、次のコマンドで確認できます。

```bash
aws configure get region
aws sts get-caller-identity --query Account --output text
```

## 2. AWS IoTポリシーを作成する

AWS IoTコンソールで、現在の公式手順では **Security → Policies → Create policy** を選択します。画面名が異なる場合は、[AWS公式のIoTリソース作成手順](https://docs.aws.amazon.com/iot/latest/developerguide/create-iot-resources.html)を参照してください。

ポリシー名の例は`PressureLoggerPolicy`です。ポリシードキュメントには次を設定します。

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:YOUR_REGION:YOUR_ACCOUNT_ID:client/PressureLogger"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": [
        "arn:aws:iot:YOUR_REGION:YOUR_ACCOUNT_ID:topic/pressure_logger/data"
      ]
    }
  ]
}
```

`YOUR_REGION`と`YOUR_ACCOUNT_ID`は実際の値へ置き換えてください。

このプロジェクトはAWSからの受信を実装していないため、`iot:Subscribe`と`iot:Receive`は不要です。クライアントIDには`thing_name`が使われるため、`client/PressureLogger`と一致させます。ARNと最小権限の考え方は[AWS公式のConnect/Publishポリシー例](https://docs.aws.amazon.com/iot/latest/developerguide/connect-and-pub.html)を参照してください。

## 3. Thingとデバイス証明書を作成する

現在の公式手順では **All devices → Things** から作成します。

1. **Create things**を選択する。
2. **Create a single thing**を選択する。
3. Thing名に`PressureLogger`を入力する。
4. 証明書設定で **Auto-generate a new certificate (recommended)** を選択する。
5. `PressureLoggerPolicy`を選択する。
6. Thingを作成する。
7. 表示された証明書と秘密鍵を、画面を閉じる前にダウンロードする。

必要なファイルは次のとおりです。

| ファイル | 用途 |
| --- | --- |
| Device certificate | デバイス証明書。通常は`*-certificate.pem.crt` |
| Private key | 秘密鍵。通常は`*-private.pem.key` |
| Amazon Root CA 1 | AWS IoTエンドポイントの検証 |
| Public key | このプロジェクトでは使用しない |

証明書作成画面を離れると秘密鍵は再ダウンロードできません。AWS公式の最新手順と注意事項は[AWS IoTリソースの作成](https://docs.aws.amazon.com/iot/latest/developerguide/create-iot-resources.html)を確認してください。

Amazon Root CA 1は公式URLから取得できます。

- [Amazon Root CA 1](https://www.amazontrust.com/repository/AmazonRootCA1.pem)

### リソースを別々に作成した場合

証明書の詳細画面で次を確認します。

- 証明書の状態が **Active**
- `PressureLogger` Thingがアタッチされている
- `PressureLoggerPolicy`がアタッチされている

現行手順は[Thingまたはポリシーを証明書へアタッチする方法](https://docs.aws.amazon.com/iot/latest/developerguide/attach-to-cert.html)を参照してください。

## 4. デバイスデータエンドポイントを確認する

AWS CLIではATSデータエンドポイントを取得できます。

```bash
aws iot describe-endpoint \
  --endpoint-type iot:Data-ATS \
  --region YOUR_REGION \
  --query endpointAddress \
  --output text
```

出力例:

```text
a1b2c3d4e5f6g7-ats.iot.ap-northeast-1.amazonaws.com
```

AWS IoTコンソールでは、現在の[公式エンドポイント確認手順](https://docs.aws.amazon.com/iot/latest/developerguide/iot-quick-start-test-connection.html)は **Connect → Domain Configurations** で **Domain name** を確認する導線です。公式資料にはSettings経由の記述も残っています。画面名だけで判断せず、同じリージョンの`describe-endpoint --endpoint-type iot:Data-ATS`の結果を基準にしてください。

設定には`https://`や`mqtts://`を付けず、ホスト名だけを使用します。

## 5. ローカル設定ファイルを作成する

```bash
cp secure/config.h.example secure/config.h
cp secure/aws_certificates.h.example secure/aws_certificates.h
```

これらの実ファイルは`.gitignore`の対象です。秘密鍵をGitへコミットしないでください。

### `secure/config.h`

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* aws_iot_endpoint =
    "a1b2c3d4e5f6g7-ats.iot.ap-northeast-1.amazonaws.com";
const int aws_iot_port = 8883;
const char* thing_name = "PressureLogger";
const char* aws_iot_topic = "pressure_logger/data";
```

`aws_iot_topic`が実際の送信先です。変更する場合は、上のポリシーの`topic/pressure_logger/data`と、MQTTテストクライアントの購読先も同じ値へ合わせてください。

### `secure/aws_certificates.h`

デバイス証明書と秘密鍵を、PEMのBEGIN/END行を含めて設定します。

```cpp
const char* device_cert = R"EOF(
-----BEGIN CERTIFICATE-----
デバイス証明書の内容
-----END CERTIFICATE-----
)EOF";

const char* device_key = R"EOF(
-----BEGIN RSA PRIVATE KEY-----
秘密鍵の内容
-----END RSA PRIVATE KEY-----
)EOF";
```

秘密鍵のBEGIN行は、ダウンロードしたファイルの形式を変更せず使用してください。`secure/aws_certificates.h.example`にはAmazon Root CA 1が含まれていますが、公式から取得した内容と一致することを確認してください。

## 6. ビルドして書き込む

```bash
git submodule update --init --recursive
pio run
pio run --target upload
pio device monitor
```

シリアルモニターは115200 baudです。正常に接続すると、次のようなメッセージが表示されます。

```text
connected to AWS IoT Core
```

LCDの`WiFi`と`AWS`表示も緑になります。NTPは非同期に同期し、時計の確定後にTLS接続を開始します。NTP不達でも起動処理は待機しません。

## 7. MQTTメッセージを確認する

現在の公式手順では、AWS IoTコンソールの **Test → MQTT test client** を開きます。画面が異なる場合は、[AWS公式のMQTTテストクライアント手順](https://docs.aws.amazon.com/iot/latest/developerguide/view-mqtt-messages.html)を参照してください。

1. Thingと同じリージョンを選択していることを確認する。
2. **Subscribe to a topic**を開く。
3. Topic filterに`pressure_logger/data`を入力する。
4. **Subscribe**を選択する。
5. M5StackのAボタンを押して記録状態にする。

この実装は記録状態のときだけMQTT Publishを行います。待機状態ではAWSへ接続済みでも送信しません。

正常なら、約500 msごとに`pressure_logger/data`へ両チャンネルを含むJSONが1件届きます。`ch0`と`ch1`の単位はMPaで、波形表示の0〜0.5 MPaという範囲には制限しません。

```json
{
  "timestamp": 123456789,
  "device": "PressureLogger",
  "session": 1,
  "sequence": 50,
  "ch0": 0.1234,
  "ch1": 0.2345
}
```

`timestamp`はUnix時刻ではなく、取得時の起動後64bitミリ秒です。`session`と`sequence`は起動中の記録セッションとサンプルの番号です。

## 8. A-03の通信分離を確認する

テスト用のWi-Fi環境と証明書で行ってください。

1. SD記録とMQTT受信が動作していることを確認する。
2. 記録中にWi-Fiアクセスポイントを一時停止する。
3. シリアルで再接続処理を確認する。
4. Wi-Fiを再開する。
5. `connected to AWS IoT Core`が再表示され、MQTT受信が再開することを確認する。
6. シリアルの`Session ... storage queue high-water ... total dropped ...`とCSV末尾のセッション別欠落数を記録する。

A-03はセンサー取得を通信保守タスクから分離しますが、AWS切断中のMQTTデータを再送する機能ではありません。切断中のクラウドデータは欠測します。保存FIFOは512件（開始／停止のための予約枠を含む）で、100 Hzなら約5秒分です。UI・SD保存・通信は別経路で動作し、通信断中の全サンプルを保持するキューではありません。通信断の長さ、欠測数、再接続までの時間を記録してください。

MQTTはQoS 0で最新値を約500 ms間隔に配信します。100 Hzの全履歴を送る仕様ではありません。古い値の再送はせず、1秒を越えた値は送信しません。送信失敗でも試行間隔を500 ms空け、成功回数・成功時刻は失敗時に更新しません。PubSubClientの成功はAWS側での保存を保証する受領確認ではありません。セッション番号は再起動でリセットされます。

## トラブルシューティング

### Wi-Fiへ接続できない

- `ssid`と`password`を確認する。
- LCDの`WiFi!`表示を確認する。再接続は非同期に5秒間隔で要求する。

### AWS表示が赤い、またはMQTT接続に失敗する

- `aws_iot_endpoint`が同じリージョンのATSデータエンドポイントか確認する。
- エンドポイントにプロトコル名やパスを付けていないことを確認する。
- `thing_name`、クライアントID、ポリシーの`client/PressureLogger`が一致していることを確認する。
- 証明書がActiveで、Thingとポリシーがアタッチされていることを確認する。
- 証明書・秘密鍵・Root CAの組み合わせを確認する。
- NTP同期とポート8883への外向き通信を確認する。

### 接続済みだがメッセージが届かない

- M5Stackが記録状態か確認する。
- MQTTテストクライアントのリージョンと`pressure_logger/data`を確認する。
- ポリシーが設定した`pressure_logger/data`への`iot:Publish`を許可していることを確認する。
- シリアルの`MQTT publish failed`と受信側の`session`・`sequence`を確認する。

### `Not authorized`になる

- ポリシーのリージョン、アカウントID、クライアントID、トピックARNを確認する。
- IAMポリシーではなく、AWS IoTポリシーがデバイス証明書へアタッチされていることを確認する。

## セキュリティ上の注意

- 秘密鍵と実際の`secure/*.h`をコミットしない。
- 開発者・デバイスごとに証明書を分ける。
- 不要な証明書は無効化または削除する。
- 秘密鍵を公開した場合はAWS IoT側で直ちに証明書を無効化し、新しい証明書を発行する。
- 本番環境では`Resource: "*"`を避け、必要なクライアントIDとトピックだけを許可する。

## AWS公式資料

- [AWS IoTリソースの作成](https://docs.aws.amazon.com/iot/latest/developerguide/create-iot-resources.html)
- [デバイス通信プロトコルとポート](https://docs.aws.amazon.com/iot/latest/developerguide/protocols.html)
- [AWS IoTデバイスエンドポイント](https://docs.aws.amazon.com/iot/latest/developerguide/iot-connect-devices.html)
- [Connect/Publishポリシー例](https://docs.aws.amazon.com/iot/latest/developerguide/connect-and-pub.html)
- [Thing・ポリシーと証明書のアタッチ](https://docs.aws.amazon.com/iot/latest/developerguide/attach-to-cert.html)
- [MQTTテストクライアント](https://docs.aws.amazon.com/iot/latest/developerguide/view-mqtt-messages.html)
