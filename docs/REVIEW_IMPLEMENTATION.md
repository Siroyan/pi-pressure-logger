# 2026-09-24レビューの実装結果

対象は[再レビュー17項目](repository-review-2026-09-24.md)。前回・今回のレビューレポートは変更せず、本書で実装と検証を追跡する。

## PRの分割と取り込み順

| ランク | ブランチ | PRのbase | 内容 |
| --- | --- | --- | --- |
| B | `fix/r2-b-reliability` | `dev` | 最初に実装したB09のテスト基盤とB01〜B08、依存固定・RTC読取の追補 |
| A | `fix/r2-a-recording` | `fix/r2-b-reliability` | A01〜A05とSD再マウント設定の追補 |
| C | `fix/r2-c-maintenance` | `fix/r2-a-recording` | C01〜C03、統合試験と本書 |

着手順はB09→A→残りのB→C。テスト基盤をBのPRに含め、各PRの差分をランク別にするため、最終的な依存順はB→A→Cとした。各指摘を別コミットにし、追補と検証済みブランチの統合は追加コミットにした。BをdevへマージしたらAのbaseをdevへ、AをマージしたらCのbaseをdevへ変更して取り込む。mainへ直接push・mergeは行わない。

## 項目ごとの証拠

| ID | 実装 | 主な回帰試験／確認 |
| --- | --- | --- |
| R2-A01 | ヘッダ・本文・CRLFの完全長を検査。失敗ファイルへの追記停止、新セッションで再マウント。GPIO4を明示 | `storage_test.cpp`の全切断長・ヘッダ／flush失敗、`sd_mount_test.cpp`の初回／復旧CSピン |
| R2-A02 | 64bit取得時刻を保存まで伝搬。CSVは開始境界との差、MQTTは取得時の起動後ms | `storage_test.cpp`の32bit周回境界・遅延消費、`mqtt_test.cpp`のペイロード |
| R2-A03 | 開始・取得・停止をFIFOで順序づけ、セッションIDと連番を付与 | `session_test.cpp`の消費停止中の開始→停止→再開始、境界枠の予約 |
| R2-A04 | 保存・通信・UIを分離。ファイル保持、1KBまとめ書き、定期flush。SPI待ち中の描画はスキップ | `pipeline_test.cpp`と`application_test.cpp`の書込中の停止、独立メールボックス、欠落数、open/close回数 |
| R2-A05 | I2C成否と8msのペア取得期限を検査。無効ペアを抑止して停止し、1秒ごとに復旧を確認 | `adc_test.cpp`の全転送段階失敗・変換停止・設定不一致・時計周回・復帰 |
| R2-B01 | 非同期Wi-Fi状態を毎回更新。接続待機ループを撤去 | `network_test.cpp`の遅延接続・再接続・周回 |
| R2-B02 | RTCを一度読む方式と非同期SNTP要求。接続復帰で再試行し、遅延成功を反映 | `network_test.cpp`のオフライン起動・NTP不達・遅延成功、SDKの10ms待機を使わないRTCアダプタ |
| R2-B03 | /data、MPa・未制限値、設定抜粋、接続導線、I2C配線を更新 | README／AWS_SETUPの旧トピック・旧圧力制限を検索。AWS・M5Stack・TI公式資料を参照 |
| R2-B04 | 画面初期化時に数値キャッシュを無効化 | `display_test.cpp`の一定入力での連続画面復帰 |
| R2-B05 | 取得時刻で列を決め、列内の最小／最大を保持。欠測・移動空白帯・2px描画を維持 | `display_test.cpp`の短いピーク・欠測・停止・古い滞留値・端点 |
| R2-B06 | 完全な名前・サイズの確認、取消、非同期削除、成功／失敗表示 | `display_test.cpp`の対象固定と失敗表示、`application_test.cpp`の実ボタン列、`file_cache_test.cpp`の削除失敗 |
| R2-B07 | キュー・保存用mutex・各タスク・MQTTバッファの失敗を確認し、RECを抑止 | `startup_test.cpp`の部分確保・MQTT資源失敗、`application_test.cpp`でsetup経由の資源・タスク失敗と画面利用可否 |
| R2-B08 | 通信タスクが送信判定と結果を管理。最新値のみ、試行間隔と成功時刻を分離 | `mqtt_test.cpp`の失敗・送信停滞中200件投入・接続遅延・停止・期限超過・周回 |
| R2-B09 | ローカルゲート、ハードウェア境界fake、ASan/UBSan、非秘密ビルド、依存バージョン固定 | `scripts/check.sh`、`requirements-dev.txt`、`platformio.ini` |
| R2-C01 | 名前・サイズをキャッシュ、数値順で比較、未同期名へ起動グループを追加 | `file_cache_test.cpp`の503件・10回再参照で追加open 0回、再起動／再マウント |
| R2-C02 | offline環境では設定・証明書・通信オブジェクト・通信タスクを不要にする | `network_mode_test.cpp`の通信呼出し0回とSD記録、offlineファームウェアビルド |
| R2-C03 | LoggerApplicationが寿命を所有。不要な共有配列・関数を削除し、同期方法を明示 | `application_test.cpp`のボタンからワーカーまでの統合試験、旧グローバル・volatileの残存検索 |

テストファイルは全て[test/host](../test/host/)配下。操作手順、故障時の振る舞い、合格基準は[LOCAL_TESTS.md](LOCAL_TESTS.md)を参照。

## 検証範囲と運用上の限界

`bash scripts/check.sh`で47ケースをonline／offlineの両方で実行し、ESP32のvalidation／offlineビルドを確認する。各コミット前にも、その時点のゲートを実行した。GitHub Actionsは追加していない。検証ビルドは公開ダミー設定またはネットワークなしの構成だけを使用し、secure内の実設定・鍵・証明書は操作していない。

実機への書込み、実ADC抜去、SD媒体の遅延測定・電源断、AWSへの実接続は未実施。100Hzは目標周期であり実測保証ではない。SDKのflush/closeが報告できない媒体内部の失敗や電源断時の永続化は保証できない。MQTTはQoS 0の最新値配信で、全サンプルの完全配送や履歴再送は行わない。これらをホスト試験で確認済みと扱わず、実機確認手順をLOCAL_TESTSに残している。
